from distutils.core import setup, Extension

module1 = Extension('memtestcext',
        sources = ['memtestcext.c'])

module2 = Extension('memtest86pcext',
        sources = ['memtest86pcext.c','algo.c','error.c'])

setup (name = 'MemTestCExt',
        version = '0.1',
        description = 'C extension for performant memory testing',
        ext_modules = [module1])

setup (name = 'Memtest86+ C-ext',
        version = '0.1',
        description = 'C extension for performant memory testing',
        ext_modules = [module2])
