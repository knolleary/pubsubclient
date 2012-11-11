## Arduino Client for MQTT Test Suite

This is a regression test suite for the `PubSubClient` library.

Without a suitable arduino plugged in, the test suite will only check the
example sketches compile cleanly against the library.

With an arduino plugged in, each sketch that has a corresponding python
test case is built, uploaded and then the tests run.

## Dependencies

 - Python 2.7+
 - [INO Tool](http://inotool.org/) - this provides command-line build/upload of Arduino sketches

## Running

Without a suitable arduino plugged in, the test suite will only check the
example sketches compile cleanly against the library.

With an arduino plugged in, each sketch that has a corresponding python
test case is built, uploaded and then the tests run. 
 
   $ python testsuite.py

## What it does

For each example sketch, `sketch.ino`, the suite looks for a matching test case
`testcases/sketch.py`.

The test case must follow these conventions:
 - sub-class `unittest.TestCase`
 - provide the class methods `setUpClass` and `tearDownClass` (TODO: make this optional)
 - all test method names must begin with `test_`
 
The suite will call the `setUpClass` method _before_ uploading the sketch. This
allows any test setup to be performed before the the sketch runs - such as connecting
a client and subscribing to topics.


## Settings

The file `testcases/settings.py` is used to config the test environment.

 - `server_ip` - the IP address of the broker the client should connect to
 - `arduino_ip` - the IP address the arduino should us

Before each sketch is compiled, these values are automatically substituted in. To
do this, the suite looks for lines that match the following:

     byte server[] = {
     byte ip[] = {




