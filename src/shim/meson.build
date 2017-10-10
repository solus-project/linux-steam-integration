# TODO: Expose this as an option!
shim_target_name = 'steam'

shim_sources = [
    'lsi.c',
    'shim.c',
]

main_intercept = executable(
    shim_target_name,
    sources: shim_sources + nica_sources,
    include_directories: nica_includes,
    install: true,
)

