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

# Ensure AWS Environment Variables exist
if 'AWS_ACCESS_KEY_ID' not in os.environ or 'AWS_SECRET_ACCESS_KEY' not in os.environ:
    print("Ensure 'AWS_ACCESS_KEY_ID' and 'AWS_SECRET_ACCESS_KEY' are properly defined environment variables")
    sys.exit(1)


# Program Variables
SYMBOL_FILE = "Etterna.sym"
AWS_BUCKET_NAME = "etterna"
AWS_ACCESS_KEY_ID = os.environ['AWS_ACCESS_KEY_ID']
AWS_SECRET_ACCESS_KEY = os.environ['AWS_SECRET_ACCESS_KEY']


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
    ett_arch = "x64"
    return "{}.{}/Etterna/".format(base_dir, ett_arch)


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
    upload_prefix = os.path.join('symbols', prefix_dir, build_uuid, filename)

    # Upload to S3
    upload_path = 's3://{}'.format(os.path.join(AWS_BUCKET_NAME, upload_prefix))
    s3_command = ['aws', 's3', 'cp', filename, upload_path]
    subprocess.run(s3_command)


upload_to_s3(SYMBOL_FILE)
sys.exit(0)
