import logging
import os
import re
import shutil
import subprocess
from src import config, utils


class Suite:
    name = "llvm-test-suite"

    def __init__(self):
        self.tests = []
        self.configuration_env = os.environ.copy()
        self.fake_run = False
        self.build = config.get_llvm_test_suite_build_path()
        self.suite = config.get_llvm_test_suite_path()
        self.caffe = config.get_caffe_bin()

    def get_tests(self):
        return self.tests

    def configure(self, CC, CXX, COPTS, CXXOPTS):
        output = config.get_output()

        if os.path.exists(self.build):
            shutil.rmtree(self.build)
        os.makedirs(self.build)
        self.go_to_builddir()

        self.configuration_env['CC'] = CC
        self.configuration_env['CXX'] = CXX
        self.configuration_env['LD_LIBRARY_PATH'] = self.caffe

        if CXXOPTS.find("-OW") != -1:
            self.fake_run = True
            logging.info("Fake run for Wazuhl tests")

        make_command = ['cmake', self.suite,
                        '-DCMAKE_BUILD_TYPE=Release',
                        '-DCMAKE_C_FLAGS_RELEASE={0}'.format(COPTS),
                        '-DCMAKE_CXX_FLAGS_RELEASE={0}'.format(CXXOPTS)]
        logging.info(make_command)
        with open(os.devnull, 'wb') as devnull:
            cmake_output = subprocess.Popen(make_command, env=self.configuration_env, stdout=subprocess.PIPE)
            out = cmake_output.stdout.read().decode('utf-8')
            logging.debug(out)

        logging.info("Configuration is finished")

        self.__init_tests__()

    def __init_tests__(self):
        utils.check_executable('lit')
        lit = subprocess.Popen(['lit', '--show-tests', self.build], stdout=subprocess.PIPE)
        output = lit.stdout.read().decode('utf-8')
        pattern = r'test-suite :: (.*)'
        results = re.findall(pattern, output)
        self.tests = [Test(os.path.join(self.build, test), self) for test in results]
        logging.info("Len tests (all): {}".format(len(self.tests)))
        self.tests = [test for test in self.tests if "Benchmark" in test.path]
        logging.info("Len tests (benchmark): {}".format(len(self.tests)))

    def go_to_builddir(self):
        os.chdir(self.build)
        logging.debug(self.build, " is a build dir")


class Test:
    def __init__(self, path, suite):
        self.path = path
        self.name = Test.__get_test_name__(path)
        self.suite = suite
        self.compile_time = None
        self.execution_time = None

    @staticmethod
    def __get_test_name__(path):
        _, test_file = os.path.split(path)
        test, _ = os.path.splitext(test_file)
        return test

    def compile(self):
        self.suite.go_to_builddir()
        with open(os.devnull, 'wb') as devnull:
            make_command = ['make', '-j2', self.name]
            logging.info(make_command)
            make_output = subprocess.run(make_command,  env=self.suite.configuration_env,
                                         stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            logging.debug("reading output")
            out = make_output.stdout.decode('utf-8')

            print(out)

    def run(self):
        if self.suite.fake_run:
            self.compile_time, self.execution_time = 10000, 10000
            return
        test_run = subprocess.Popen(['lit', self.path], stdout=subprocess.PIPE)
        output = test_run.stdout.read().decode('utf-8')
        compile_pattern = r'compile_time: (.*)'
        execution_pattern = r'exec_time: (.*)'
        compile_time = re.search(compile_pattern, output)
        execution_time = re.search(execution_pattern, output)
        if compile_time:
            compile_time = float(compile_time.group(1))
        if execution_time:
            execution_time = float(execution_time.group(1))
        self.compile_time, self.execution_time = compile_time, execution_time

    def __str__(self):
        return self.name
