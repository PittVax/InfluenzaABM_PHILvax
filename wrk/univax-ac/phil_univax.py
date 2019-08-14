# coding: utf-8

import sh
import os
import re
import sys
import time
import argparse
import jinja2
import stopit

import pandas as pd

from tempfile import mkdtemp
from random import randint
from collections import OrderedDict, namedtuple
from datetime import datetime

class log(object):
    @staticmethod
    def info(s):
        print(s)
    @staticmethod
    def warn(s):
        print(s)
    @staticmethod
    def error(s):
        print(s)

class PhilUniversalExperiment(object):

    synthetic_population_directory = os.path.join(os.environ['PHIL_HOME'], 'populations')
    synthetic_population_id = '2005_2009_ver2_42003'
    synthetic_population = os.path.join(synthetic_population_directory, synthetic_population_id)

    base_params = {
            'synthetic_population_directory': synthetic_population_directory,
            'synthetic_population_id': synthetic_population_id
            }

    def __init__(self):
        self.wrkdir = os.getcwd()
        self.phil_home = os.environ['PHIL_HOME']
        self.base_param_file = 'params.seasonal'
        self.qsub_template_file = 'qsub.tpl'

    def run(self, base_paramfile, opt_params, run_name):
        for i in range(3):
            try:
                with stopit.ThreadingTimeout(60*20) as timeout_mgr:
                    assert timeout_mgr.state == timeout_mgr.EXECUTING
                    tempdir, poe_output_file = self.run_phil_pipeline(base_paramfile, opt_params, run_name)
                if timeout_mgr.state == timeout_mgr.EXECUTED:
                    return True
                else:
                    raise Exception('Timeout!')
            except Exception as e:
                # this is a hack.  there are occasional filesystem errors that cause 
                # approx 1 in 10,000 executions to fail silently (and the island evolve
                # method doesn't handle the exception at all).  So just sleep and try again
                time.sleep(30)
        return False

    def read_phil_base_params_from_file(self, filename=None):
        p = {}
        _filename = self.base_param_file if filename is None else filename
        with open(_filename, 'r') as f:
            for l in f:
                l = l.strip()
                if not l.startswith('#') and len(l) > 0 and '=' in l:
                    l = re.sub(r'(?m)^ *#.*\n?', '', l)
                    m = re.search('^(.+?) = (.+)$', l)
                    if m is not None:
                        p[m.group(1)] = m.group(2)
        return p

    def run_phil_pipeline(self, base_paramfile, opt_params, run_name):
        tempdir_container = os.path.join(self.wrkdir, 'phil_univax_out', base_paramfile, '%s.%d' % (run_name, randint(0,sys.maxsize)))
        sh.mkdir('-p',tempdir_container)
        tempdir = mkdtemp(prefix='phl-', dir=tempdir_container)
        basename = os.path.basename(tempdir)
        event_report_file = os.path.join(tempdir, 'events.json_lines')
        poe_output_file = os.path.join(tempdir, 'poe_output')
        poe_format = 'csv'
        qsub = sh.qsub.bake('-h','-v','PHIL_HOME=%s,OMP_NUM_THREADS=16' % self.phil_home)

        paramfile = open(os.path.join(tempdir,'params'), 'w')
        params = self.read_phil_base_params_from_file(base_paramfile)
        params.update({
            'outdir': tempdir,
            'event_report_file' : event_report_file,
            'seed': randint(1, 2147483647)
        })
        params.update(opt_params)

        for param, value in params.items():
            paramfile.write('%s = %s\n' % (param, str(value)))
        paramfile.close()

        lockfile = os.path.join(tempdir, 'lockfile')
        statusfile = os.path.join(tempdir, 'statusfile')

        sh.cp(params['primary_cases_file[0]'], tempdir)
        sh.cp(params['vaccination_capacity_file'], tempdir)
        sh.cp('config.yaml', tempdir)

        qsub_template_args = dict(
            stdout = os.path.join(tempdir, 'stdout'),
            stderr = os.path.join(tempdir, 'stderr'),
            lockfile = lockfile, statusfile = statusfile,
            tempdir = tempdir, jobname = basename,
            #reservation = 'philo.0',
            paramfile = paramfile.name,
            synthetic_population = self.synthetic_population,
            event_report_file = event_report_file,
            poe_output_file = poe_output_file, poe_format = poe_format)

        with open(self.qsub_template_file, 'r') as f:
            qsub_template = jinja2.Template(f.read())
      
        qsub_file = os.path.join(tempdir, 'qsub.py')
        with open(qsub_file, 'w') as f:
            f.write(qsub_template.render(qsub_template_args))

        jobid = qsub(qsub_file).strip()
        sh.ln('-s', tempdir, os.path.join(tempdir_container, jobid))
        sh.touch(lockfile)
        sh.qalter('-h','n', jobid)

        while sh.qstat('-x', jobid, _ok_code=[0,153]).exit_code == 0:
            time.sleep(randint(1,4))

        n_check = 3
        for _n in range(n_check+1):
            try:
                if os.path.isfile(lockfile):
                    raise Exception('Lockfile present but %s not in queue!' % jobid)

                with open(statusfile, 'r') as f:
                    stat = f.read()
                    if len(stat) > 0:
                        raise Exception(stat)
                break
            except Exception as e:
                if _n == n_check:
                    raise(e)
                else:
                    time.sleep(randint(10,20))

        return (tempdir, '%s.%s' % (poe_output_file, poe_format))
















