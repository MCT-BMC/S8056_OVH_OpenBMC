project(
    'rotate-event-logs',
    'cpp',
    default_options: [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++17',
    ],
    license: 'MIT',
    version: '1.0.0',
)

boost = dependency('boost')
phosphor_logging = dependency('phosphor-logging')

boost_args = ['-DBOOST_ASIO_DISABLE_THREADS',
              '-DBOOST_ERROR_CODE_HEADER_ONLY',
              '-DBOOST_SYSTEM_NO_DEPRECATED']

executable(
    'rotate-event-logs',
    'rotate-event-logs.cpp',
    dependencies: [
        boost,
        phosphor_logging
    ],
    cpp_args: [boost_args],
    install: true,
    install_dir: get_option('bindir')
)
