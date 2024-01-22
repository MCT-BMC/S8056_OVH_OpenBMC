project(
    'boot-option',
    'cpp',
    default_options: [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++20',
    ],
    license: 'MIT',
    version: '1.0.0',
)

boost = dependency('boost')
phosphor_logging = dependency('phosphor-logging')
sdbusplus = dependency('sdbusplus')
threads = dependency('threads')

boost_args = [
    '-DBOOST_SYSTEM_NO_DEPRECATED',
    '-DBOOST_ERROR_CODE_HEADER_ONLY',
    '-DBOOST_NO_RTTI',
    '-DBOOST_NO_TYPEID',
    '-DBOOST_ALL_NO_LIB',
]

executable(
    'boot-option',
    'boot-option.cpp',
    dependencies: [
        boost,
        phosphor_logging,
        sdbusplus,
        threads
    ],
    cpp_args : [
        boost_args
    ],
    install: true,
    install_dir: get_option('bindir'),
)
