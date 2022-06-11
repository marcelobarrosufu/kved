import os

AddOption('--compiler-prefix',
            dest='compiler_prefix',
            type='string',
            nargs=1,
            action='store',
            metavar='DIR',
            default='',
            help='compiler prefix')
            
AddOption('--target',
            dest='target',
            type='string',
            nargs=1,
            action='store',
            metavar='DIR',
            default='simul',
            help='target project/port to be compiled')

compiler_prefix = GetOption('compiler_prefix')
target = GetOption('target')

env_options = {
    "CC"    : compiler_prefix + "gcc",
    "CXX"   : compiler_prefix + "g++",
    "LD"    : compiler_prefix + "g++",
    "AR"    : compiler_prefix + "ar",
    "STRIP" : compiler_prefix + "strip",
    "PATH"  : os.environ['PATH'],
	"CCFLAGS" : ['-pedantic','-std=c11','-g','-Wall','-D_GNU_SOURCE ','-DSIMUL'],
    "CPPPATH" : ['.'],
	"LINKFLAGS" : ['-Wall'],
}

env = Environment(**env_options)

env['ENV']['TERM'] = os.environ['TERM']

common_source = ['kved.c','kved_cpu.c']
common_include = ['kved.h','kved_flash.h','kved_cpu.h','kved_config.h']
target_source = []
target_include = []

if target == 'simul':
    target_source = ['./port/simul/port_flash.c','./test/kved_test.c','./test/kved_test_main.c']
    target_include = ['./port/simul/kved_test.c']
    env["CCFLAGS"].append('-DKVED_DEBUG')

srcs = common_source + target_source
incs = common_include + target_include

env.Program('kved',srcs,incs)

