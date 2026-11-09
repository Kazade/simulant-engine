#!/bin/bash
#
# Runs SimulantTests.app (built by build_libraries.sh with IOS_BUILD_TESTS=1,
# see tests/CMakeLists.txt) in a new iOS simulator, on a Mac with Xcode, and
# fails if any test does.
#
# Usage: run_simulator_tests.sh <SimulantTests.app> <JUnit report to write>
#
# The simulator is the newest iOS one installed, on an iPhone it supports,
# and is deleted afterwards. TEST_TIMEOUT (seconds, default 1800) limits how
# long the tests can take. Arguments after the first two are passed to the
# test runner (e.g. a test's name, to run only that one).
#

set -euo pipefail

APP=${1:?Usage: $0 <SimulantTests.app> <report.xml> [test runner arguments]}
REPORT=${2:?Usage: $0 <SimulantTests.app> <report.xml> [test runner arguments]}
shift 2
TEST_TIMEOUT=${TEST_TIMEOUT:-1800}

BUNDLE_ID=$(/usr/libexec/PlistBuddy -c "Print :CFBundleIdentifier" "${APP}/Info.plist")

# The newest iOS runtime, and the newest iPhone it supports
read -r RUNTIME DEVICE_TYPE < <(xcrun simctl list -j runtimes | python3 -c '
import json, sys

runtimes = [
    r for r in json.load(sys.stdin)["runtimes"]
    if r.get("isAvailable") and (r.get("platform") == "iOS" or r["name"].startswith("iOS"))
]
if not runtimes:
    sys.exit("There are no iOS simulator runtimes installed")

runtime = max(runtimes, key=lambda r: [int(n) for n in r["version"].split(".")])
phones = [
    d for d in runtime.get("supportedDeviceTypes", [])
    if d.get("productFamily") == "iPhone"
]
if not phones:
    sys.exit("%s supports no iPhones" % runtime["name"])

print(runtime["identifier"], phones[-1]["identifier"])
')
echo "Testing on ${DEVICE_TYPE##*.} with ${RUNTIME##*.}"

DEVICE=$(xcrun simctl create "Simulant Tests" "${DEVICE_TYPE}" "${RUNTIME}")
cleanup() {
    xcrun simctl shutdown "${DEVICE}" > /dev/null 2>&1 || true
    xcrun simctl delete "${DEVICE}" > /dev/null 2>&1 || true
}
trap cleanup EXIT

xcrun simctl boot "${DEVICE}"
xcrun simctl bootstatus "${DEVICE}" -b > /dev/null

# The simulator only installs apps whose bundle is signed (resources and
# all); without a certificate, that's an ad-hoc signature
codesign --force --sign - --timestamp=none "${APP}"
xcrun simctl install "${DEVICE}" "${APP}"

# The report's written in the app's own data container (the simulator's
# files are the Mac's), as the bundle's the working directory and the app
# can't write anywhere else
DATA=$(xcrun simctl get_app_container "${DEVICE}" "${BUNDLE_ID}" data)
mkdir -p "${DATA}/Documents"
RESULT=${DATA}/Documents/report.xml
rm -f "${RESULT}"

# --console-pty waits for the app to exit, and shows its output.
# SIMCTL_CHILD_* variables are passed to the app (without the prefix).
set +e
SIMCTL_CHILD_SIMULANT_SOUND_DRIVER=null \
    perl -e 'alarm shift; exec @ARGV' "${TEST_TIMEOUT}" \
    xcrun simctl launch --console-pty --terminate-running-process \
    "${DEVICE}" "${BUNDLE_ID}" --junit-xml "${RESULT}" "$@"
STATUS=$?
set -e

if [ ! -f "${RESULT}" ]; then
    echo "The tests didn't finish (simctl exited with ${STATUS}): the app crashed, or took over ${TEST_TIMEOUT}s"

    # Crash reports, to keep as artifacts
    mkdir -p crash-reports
    find ~/Library/Logs/DiagnosticReports -name 'simulant_tests*' -mmin -60 \
        -exec cp {} crash-reports/ \; 2> /dev/null || true
    exit 1
fi

mkdir -p "$(dirname "${REPORT}")"
cp "${RESULT}" "${REPORT}"

# The runner's exit code doesn't come back through simctl, so the report
# says whether everything passed
python3 - "${REPORT}" <<'EOF'
import sys
import xml.etree.ElementTree as ET

cases = list(ET.parse(sys.argv[1]).getroot().iter("testcase"))
failed = [c for c in cases if c.find("failure") is not None or c.find("error") is not None]
print("%d tests ran, %d failed" % (len(cases), len(failed)))
for case in failed:
    print("  FAILED: %s" % case.get("name"))
sys.exit(1 if failed or not cases else 0)
EOF
