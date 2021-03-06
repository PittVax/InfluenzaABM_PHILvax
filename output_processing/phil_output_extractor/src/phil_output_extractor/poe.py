#!/usr/bin/env python

"""
poe: phil Output Extraction tool

Given the synthetic population files and the one or more line-list files
with json recorded events produced by phil, this script will generate
compressed hdf5-format data tables containing stratified disease state counts.
"""

__version__ = '0.0.1'

import argparse, os

#try:
from convert_output import * 
#except ImportError as e:
#    from phil_output_extractor.convert_output import *

import yaml, ujson
import logging
logging.basicConfig(level=logging.DEBUG, format='[%(name)s] %(asctime)s %(message)s')
log = logging.getLogger(__name__)

def main():
    parser = argparse.ArgumentParser(description='%s\nversion %s' % (
        __doc__, __version__),
        formatter_class=argparse.RawDescriptionHelpFormatter)
    
    parser.add_argument('-p', '--population', required=True,
            help='The synthetic population directory to use')
    
    parser.add_argument('-r', '--reportfiles', required=True,
            action='store', nargs='+',
            help='List of event report json files')

    parser.add_argument('-o', '--outfile', required=True,
            help='Base name for output file')

    parser.add_argument('-f', '--format', required=False,
            choices=['hdf','csv','apollo','galapagos'], default='csv',
            help='Format for output file')
  
    parser.add_argument('-c', '--compression', required=False,
            choices=['bz2','gzip','none'], default='bz2',
            help='Format for output file')

    parser.add_argument('-g', '--groupconfig', type=file, required=False,
            help=' '.join(['Optional yaml file specifying stratification groups.',
            'If not specified, the default behavior is to stratify by integer age,',
            'and tract-level location']))

    args = parser.parse_args()

    output_collection = OutputCollection(args.population)

    try:
        groupconfig = yaml.load(args.groupconfig)
    except:
        groupconfig = yaml.load(output_collection.default_config)
        log.info('No grouping config file supplied, using defaults: %s' % (
            ujson.dumps(groupconfig),))

    if args.format == 'hdf':
        output_collection.write_event_counts_to_hdf5(args.reportfiles, args.outfile, groupconfig)
    if args.format == 'csv': 
        output_collection.write_event_counts_to_csv(args.reportfiles, args.outfile, groupconfig)
    if args.format == 'apollo': 
        output_collection.write_apollo_internal(args.reportfiles, args.outfile, groupconfig)
    if args.format == 'galapagos':
        output_collection.write_galapagos(args.reportfiles, args.outfile, groupconfig)


if __name__ == '__main__':
    main()
