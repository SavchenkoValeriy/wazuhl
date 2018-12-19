import logging
import random
from tqdm import tqdm

from src import config


def get_tests(suites, flags):
    tests = []
    for suite in suites:
        logging.info("Suite: {}".format(suite))
        suite.configure(config.get_clang(), config.get_clangpp(), flags, flags)
        logging.info("Configured suite and preparing to get tests from it")
        tests.extend(suite.get_tests())
        logging.info(len(suite.get_tests()))
        logging.info(len(tests))
    return tests


def run(tests):
    for test in tqdm(tests):
        run_test(test)
    return tests


def run_random(tests):
    assert len(tests) > 0, "Please provide some tests to run_random"
    logging.debug("Running some random test")
    test = random.choice(tests)
    logging.debug("Chosen a test")
    return run_test(test)


def run_test(test):
    logging.debug("Compile test")
    test.compile()
    logging.debug("Run test")
    test.run()
    return test
