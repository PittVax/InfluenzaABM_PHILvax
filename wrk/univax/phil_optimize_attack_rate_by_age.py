# coding: utf-8

import sh
import os
import re
import time
import argparse
import jinja2
import stopit

import pandas as pd

from tempfile import mkdtemp
from random import randint
from collections import OrderedDict, namedtuple
from datetime import datetime
from PyGMO.problem import *


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

class PhilOptimizeAttackRateByAge(base):
    age_groups = ['[0, 5)','[5, 18)','[18, 50)',
                  '[50, 65)','[65, 106)']

    synthetic_population_directory = os.path.join(os.environ['PHIL_HOME'], 'populations')
    synthetic_population_id = '2005_2009_ver2_42003'
    synthetic_population = os.path.join(synthetic_population_directory, synthetic_population_id)

    base_params = {
                'synthetic_population_directory': synthetic_population_directory,
                'synthetic_population_id': synthetic_population_id,
            }

    optimized_param_array_defaults = [
        0.198226,  # household_contacts[0]
        42.478577, # neighborhood_contacts[0]
        14.320478, # school_contacts[0]
        1.589467,  # workplace_contacts[0]
        14.320478, # classroom_contacts[0]
        1.589467,  # office_contacts[0]
        1.5,       # weekend_contact_rate[0]

        # household_prob[0] = 4
        0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3,
        # workplace_prob[0] = 1
        0.0575,
        # office_prob[0] = 1
        0.0575,
        # school_prob[0] = 16
        0.0435, 0, 0, 0.0435, 0, 0.0375, 0, 0.0375, 0, 0, 0.0315, 0.0315, 0.0435, 0.0375, 0.0315, 0.0575,
        # classroom_prob[0] = 16
        0.0435, 0, 0, 0.0435, 0, 0.0375, 0, 0.0375, 0, 0, 0.0315, 0.0315, 0.0435, 0.0375, 0.0315, 0.0575,
        # neighborhood_prob[0] = 4
        0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3,
      
        1.0,       # trans[0]
        ]

    idx = namedtuple('ArrayIndexes', ['start', 'end', 'is_indexed', 'min', 'max'])

    optimized_param_array_indexes = OrderedDict([
        ('household_contacts',    idx( 0,  1,  False,  0.00,  5.00 )),
        ('neighborhood_contacts', idx( 1,  2,  False,  0.00, 50.00 )),
        ('school_contacts',       idx( 2,  3,  False,  0.00, 20.00 )),
        ('workplace_contacts',    idx( 3,  4,  False,  0.00,  4.00 )),
        ('classroom_contacts',    idx( 4,  5,  False,  0.00, 20.00 )),
        ('office_contacts',       idx( 5,  6,  False,  0.00,  4.00 )),
        ('weekend_contact_rate',  idx( 6,  7,  False,  0.00,  2.00 )),
        ('household_prob',        idx( 7,  32, True,   0.00,  1.00 )),
        ('workplace_prob',        idx( 32, 33, True,   0.00,  1.00 )),
        ('office_prob',           idx( 33, 34, True,   0.00,  1.00 )),
        ('school_prob',           idx( 34, 50, True,   0.00,  1.00 )),
        ('classroom_prob',        idx( 50, 66, True,   0.00,  1.00 )),
        ('neighborhood_prob',     idx( 66, 91, True,   0.00,  1.00 )),
        ('trans',                 idx( 91, 92, False,  0.10,  2.00 )),
        ])

    # The annual impact of seasonal influenza in the US: Measuring disease burden and costs
    # Molinari, et al; 2007
    target = pd.DataFrame(dict(
                age = ['[0, 5)','[5, 18)','[18, 50)','[50, 65)','[65, 106)'],
                attack_rate_mean = [0.203, 0.102, 0.066, 0.066, 0.090],
                attack_rate_stddev = [0.062, 0.032, 0.017, 0.017, 0.024])
                ).set_index('age').sort_index()

    target_year = 1

    def __init__(self):
        self.wrkdir = os.getcwd()
        self.phil_home = os.environ['PHIL_HOME']
        self.base_param_file = 'params.seasonal'
        self.qsub_template_file = 'qsub.tpl'
        nobj = 2 * len(self.target.index)
        nint = 0
        ndim = 0
        lower_bounds = []
        upper_bounds = []
        for k,v in self.optimized_param_array_indexes.items():
            if v.end > ndim:
                ndim = v.end
            lower_bounds.extend([v.min] * (v.end - v.start))
            upper_bounds.extend([v.max] * (v.end - v.start))
        super(PhilOptimizeAttackRateByAge, self).__init__(ndim, nint, nobj)
        self.set_bounds(lower_bounds, upper_bounds)

    def _objfun_impl(self, x):
        for i in range(3):
            try:
                with stopit.ThreadingTimeout(60*20) as timeout_mgr:
                    assert timeout_mgr.state == timeout_mgr.EXECUTING
                    opt_params = self.build_phil_opt_params_dict_from_vec(x)
                    tempdir, poe_output_file = self.run_phil_pipeline(opt_params)
                    objective = self.evaluate_phil_output(poe_output_file, tempdir)
                if timeout_mgr.state == timeout_mgr.EXECUTED:
                    return objective
                else:
                    raise Exception('Timeout!')
            except Exception as e:
                # this is a hack.  there are occasional filesystem errors that cause 
                # approx 1 in 10,000 executions to fail silently (and the island evolve
                # method doesn't handle the exception at all).  So just sleep and try again
                time.sleep(30)
        # if we never get it, return a guaranteed crappy objective
        return [999.999] * len(self.target.index) * 2

    def build_phil_opt_params_dict_from_vec(self, x):
        p = OrderedDict()
        for k, i in self.optimized_param_array_indexes.items():
            if i.is_indexed:
                p['%s[0]' % k] = '%d %s' % (
                        i.end - i.start,
                        ' '.join(['%f' % x[j] for j in range(i.start, i.end)]))
            else:
                p['%s[0]' % k] = '%f' % x[i.start]
        return p

    def build_phil_opt_params_vec_from_dict(self, p):
        v = self.optimized_param_array_defaults[:]
        for k, i in self.optimized_param_array_indexes.items():
            _k = '%s[0]' % k
            if _k in p:
                if i.is_indexed:
                    for v_i, p_i in zip(range(i.start,i.end), range(i.end-i.start)):
                        v[v_i] = p[_k].split()[p_i+1]
                else:
                    v[i.start] = p[_k]
            else:
                print(_k)
        return [float(s) for s in v]

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

    def run_phil_pipeline(self, opt_params):
        tempdir_container = os.path.join(self.wrkdir,'philo_output',str(randint(0,32)),str(randint(0,32)))
        sh.mkdir('-p',tempdir_container)
        tempdir = mkdtemp(prefix='phl-', dir=tempdir_container)
        basename = os.path.basename(tempdir)
        event_report_file = os.path.join(tempdir, 'events.json_lines')
        poe_output_file = os.path.join(tempdir, 'poe_output')
        poe_format = 'csv'
        qsub = sh.qsub.bake('-h','-v','PHIL_HOME=%s,OMP_NUM_THREADS=16' % self.phil_home)

        paramfile = open(os.path.join(tempdir,'params'), 'w')
        params = self.read_phil_base_params_from_file()
        params.update(self.base_params)
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
            reservation = 'philo.0', paramfile = paramfile.name,
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

    def evaluate_phil_output(self, poe_output_file, tempdir):
        d1 = pd.read_csv(poe_output_file)

        def yearly_stats(s):
            return pd.Series({
                'N_p': s['N_p'].mean(),
                'IS_i': s['IS_i'].sum() / 365.0,
                'attack_rate': ((s['IS_i'] / s['N_p']).mean() * 365.0).round(3),
            })

        d2 = d1.groupby(['age']).apply(yearly_stats)
        d3 = d2.join(self.target)

        def build_objectives(_s):
            s = _s.copy()
            z = s.z.round(3).item()
            s['z_abs'] = z if z >= 0 else (-1) * z
            s_pos = s.copy()
            s_pos['objective_type'] = 'z-pos'
            s_neg = s.copy()
            s_neg['objective_type'] = 'z-neg'
            if z < 0:
                s_pos.z_abs = 0
            if z > 0:
                s_neg.z_abs = 0
            return pd.concat([s_pos,s_neg])

        d3['z'] = (d3.attack_rate - d3.attack_rate_mean) / d3.attack_rate_stddev

        d3 = d3.groupby(level=0, group_keys=False).apply(build_objectives
                         ).reset_index().rename(columns={'index':'age_index'})

        d3.to_csv(os.path.join(tempdir, 'objective.csv'))
        return d3.z_abs.tolist()
















