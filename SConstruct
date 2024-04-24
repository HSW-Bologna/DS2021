import os
import multiprocessing
from pathlib import Path
import platform
import tools.meta.csv2carray as csv2carray


def PhonyTargets(
    target,
    action,
    depends,
    env=None,
):
    # Creates a Phony target
    if not env:
        env = DefaultEnvironment()
    t = env.Alias(target, depends, action)
    env.AlwaysBuild(t)


SIMULATED_PROGRAM = "simulated"
TARGET_PROGRAM = "DS2021.bin"
MAIN = "main"
ASSETS = "assets"
COMPONENTS = "components"
LVGL = f'{COMPONENTS}/lvgl'
DRIVERS = f'{COMPONENTS}/lv_drivers'
LIGHTMODBUS = f"{COMPONENTS}/liblightmodbus"
PAR_DESCRIPTIONS = f"{MAIN}/model/descriptions"
STRING_TRANSLATIONS = f"{MAIN}/view/intl"


CFLAGS = [
    "-Wall",
    "-Wextra",
    "-Wno-unused-function",
    "-g",
    "-O0",
    "-DLV_CONF_INCLUDE_SIMPLE",
    "-DLV_LVGL_H_INCLUDE_SIMPLE",
    '-DGEL_PARAMETER_CONFIGURATION_HEADER="\\"gel_parameter_conf.h\\""',
    '-DGEL_PAGEMANAGER_CONFIGURATION_HEADER="\\"gel_pman_conf.h\\""',
    "-Wno-unused-parameter",
]

CPPPATH = [
    f"#{LVGL}/src", LVGL, DRIVERS,  f"#{MAIN}", f"#{MAIN}/config", f"{LIGHTMODBUS}/include",
    f"{COMPONENTS}/log/src"
]


TRANSLATIONS = [
    {
        "res": [f"{PAR_DESCRIPTIONS}/AUTOGEN_FILE_par.c", f"{PAR_DESCRIPTIONS}/AUTOGEN_FILE_par.h"],
        "input": f"{ASSETS}/translations/parameters",
        "output": PAR_DESCRIPTIONS,
    },
    {
        "res": [f"{STRING_TRANSLATIONS}/AUTOGEN_FILE_strings.c", f"{STRING_TRANSLATIONS}/AUTOGEN_FILE_strings.h"],
        "input": f"{ASSETS}/translations/strings",
        "output": STRING_TRANSLATIONS
    },
]


def get_target(env, name, suffix="", dependencies=[]):
    def rchop(s, suffix):
        if suffix and s.endswith(suffix):
            return s[:-len(suffix)]
        return s

    sources = [File(filename)
               for filename in Path(f"{MAIN}/").rglob('*.c')]
    sources += [
        File(filename) for filename in Path(f'{LVGL}/src').rglob('*.c')
    ]
    sources += [
        File(filename) for filename in Path(DRIVERS).rglob('*.c')
    ]
    sources += [File(f"{COMPONENTS}/log/src/log.c")]

    gel_env = env
    gel_selected = ["collections",
                    "parameter", "timer"]
    (gel, include) = SConscript(
        f'{COMPONENTS}/generic_embedded_libs/SConscript', variant_dir=f"build-{name}/gel", exports=['gel_env', 'gel_selected'])
    env['CPPPATH'] += [include]

    lv_pman_env = env
    (lv_pman, include) = SConscript( f'{COMPONENTS}/lv_page_manager/SConscript', variant_dir=f"build-{name}/lv_pman", exports=['lv_pman_env'])
    print(lv_pman, include)
    env['CPPPATH'] += [include]

    objects = [env.Object(
        f"{rchop(x.get_abspath(), '.c')}{suffix}", x) for x in sources]

    target = env.Program(name, objects + [gel, lv_pman])
    env.Depends(target, dependencies)
    env.Clean(target, f"build-{name}")
    return target


def main():
    num_cpu = multiprocessing.cpu_count()
    SetOption('num_jobs', num_cpu)
    print("Running with -j {}".format(GetOption('num_jobs')))

    translations = []
    for translation in TRANSLATIONS:
        def operation(t):
            return lambda target, source, env: csv2carray.main(t["input"], t["output"])

        t = Command(
            translation["res"], Glob(f"{translation['input']}/*.csv"), operation(translation))
        translations += t
    Alias("intl", translations)

    env_options = {
        "ENV": os.environ,
        "CPPPATH": CPPPATH,
        'CPPDEFINES': [],
        "CCFLAGS": CFLAGS + ["-DTARGET_DEBUG", "-DUSE_SDL=1"],
        "LIBS": ["-lpthread", "-lSDL2", "-larchive"]
    }

    simulated_env = Environment(**env_options)
    simulated_env.Tool('compilation_db')

    target_env = simulated_env.Clone(
        LIBS=["-lpthread", "-larchive"], CC="~/Mount/Data/Projects/new_buildroot/buildrpi3/output/host/bin/aarch64-buildroot-linux-uclibc-gcc",
        CCFLAGS=CFLAGS + ["-DUSE_FBDEV=1", "-DUSE_EVDEV=1"])

    simulated_prog = get_target(
        simulated_env, SIMULATED_PROGRAM, dependencies=["intl"])
    target_prog = get_target(target_env, "DS2021",
                             suffix="-pi", dependencies=["intl"])

    PhonyTargets('run', f"./{SIMULATED_PROGRAM}",
                 simulated_prog, simulated_env)
    compileDB = simulated_env.CompilationDatabase('compile_commands.json')

    ip_addr = ARGUMENTS.get("ip", "")
    compatibility_options = "-o StrictHostKeyChecking=no -o PubkeyAcceptedAlgorithms=+ssh-rsa"
    PhonyTargets(
        'kill-remote',
        f"ssh {compatibility_options} root@{ip_addr} 'killall gdbserver; killall app; killall sh'; true", None)
    PhonyTargets(
        "scp", f"scp -O {compatibility_options} DS2021 root@{ip_addr}:/tmp/app", [target_prog, "kill-remote"])
    PhonyTargets(
        'ssh', f"ssh {compatibility_options} root@{ip_addr}", [])
    PhonyTargets(
        'run-remote',
        f"ssh {compatibility_options} root@{ip_addr} /tmp/app",
        'scp')
    PhonyTargets(
        "update-remote",
        f'ssh {compatibility_options} root@{ip_addr} "mount -o rw,remount / && cp /tmp/app /root/app && sync && reboot && exit"', "scp")
    PhonyTargets(
        "debug",
        f"ssh {compatibility_options} root@{ip_addr} gdbserver localhost:1235 /tmp/app", "scp")
    PhonyTargets(
        "run-remote",
        f"ssh {compatibility_options} root@{ip_addr} /tmp/app",
        "scp")

    Depends(simulated_prog, compileDB)
    Default(simulated_prog)
    Alias("target", target_prog)
    Alias("all", [target_prog, simulated_prog])


main()
