#!/usr/bin/env python3
"""
Etterna Symbol Uploader
Takes the Etterna.sym file, and uploads it to the S3 at the correct
google breakpad directory location.

Google Breakpad Directory Structure is as follows: symbols/<module_id>/<build_uuid>/<module_id>.sym
Directory Structure for Etterna: symbols/Platform.Arch/Etterna/<build_uuid>/Etterna.sym
"symbols/Platform.Arch" will be the directory when passing into minidump_stackwalk
"""
import os
import sys
import platform
import subprocess
from pathlib import PurePosixPath

# Ensure AWS Environment Variables exist
if 'AWS_ACCESS_KEY_ID' not in os.environ or 'AWS_SECRET_ACCESS_KEY' not in os.environ:
    print("Ensure 'AWS_ACCESS_KEY_ID' and 'AWS_SECRET_ACCESS_KEY' are properly defined environment variables")
    sys.exit(1)

# Program Variables
SYMBOL_FILE = "Etterna.sym"
AWS_BUCKET_NAME = "etterna"
AWS_ACCESS_KEY_ID = os.environ['AWS_ACCESS_KEY_ID']
AWS_SECRET_ACCESS_KEY = os.environ['AWS_SECRET_ACCESS_KEY']
ETTERNA_ARCH = os.environ['ETTERNA_ARCH']


def get_s3_upload_directory():
    """
    Get the correct directory on AWS for symbol upload.
    Current possible symbol directories include:
        - symbols/Windows.i386/Etterna
        - symbols/Windows.x64/Etterna
        - symbols/Darwin.x64/Etterna
    :return: The correct base directory
    """
    base_dir = platform.system()
    return "{}.{}/Etterna/".format(base_dir, ETTERNA_ARCH)


def get_metadata(filename: str):
    """Read first line to get relevant file data"""
    with open(filename, 'r') as f:
        first_line = f.readline().strip()   # Remove newline
    first_line = first_line.split(' ')  # Split by spaces
    return first_line[3], first_line[4]


def upload_to_s3(filename: str):
    # Collect Metadata
    prefix_dir = get_s3_upload_directory()
    build_uuid, module_id = get_metadata(filename)
    upload_prefix = PurePosixPath('symbols', prefix_dir, build_uuid, filename)
    print("The upload_prefix is {}".format(upload_prefix))

    # Get git hash
    git_command = ['git', 'rev-parse', 'HEAD']
    process = subprocess.run(git_command, capture_output=True)
    full_hash = process.stdout.decode('utf-8').strip()

    # Upload to S3
    s3_command = [
        'aws', 's3api', 'put-object',
        '--bucket', AWS_BUCKET_NAME,
        '--body', filename,
        '--key', upload_prefix,
        '--tagging', 'GitHash={}'.format(full_hash)]
    subprocess.run(s3_command)


upload_to_s3(SYMBOL_FILE)
sys.exit(0)
