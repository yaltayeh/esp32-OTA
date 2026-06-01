#!/bin/bash

set -e

# === [Variable Setup] ===
BUCKET_NAME="esp32-ota-storage-bin-2026"

# Detect AWS region dynamically from EC2 IMDSv2 (fallback to eu-north-1)
TOKEN=$(curl -s -X PUT "http://169.254.169.254/latest/api/token" -H "X-aws-ec2-metadata-token-ttl-seconds: 60" 2>/dev/null || true)
if [ -n "$TOKEN" ]; then
    REGION=$(curl -s -H "X-aws-ec2-metadata-token: $TOKEN" http://169.254.169.254/latest/meta-data/placement/region 2>/dev/null || echo "eu-north-1")
else
    REGION="eu-north-1"
fi

BUILD_DIR="build"
VERSION_FILE="$BUILD_DIR/version.txt"
FIRMWARE_FILE="$BUILD_DIR/test_OTA.bin"

echo "==============================================="
echo " Starting After-Build Deployment Process "
echo "==============================================="

# 1. Verify build outputs
echo "--> [1/2] Verifying build outputs..."
if [ ! -f "$VERSION_FILE" ]; then
    echo "Error: version.txt not found! Ensure CMake generated it."
    exit 1
fi
if [ ! -f "$FIRMWARE_FILE" ]; then
    echo "Error: test_OTA.bin not found! Did the compilation fail?"
    exit 1
fi

# 2. Upload to AWS S3
echo "--> [2/2] Uploading binaries to AWS S3..."

echo "Uploading firmware.bin..."
aws s3 cp "$FIRMWARE_FILE" "s3://$BUCKET_NAME/firmware.bin" --region "$REGION"

echo "Uploading version.txt..."
aws s3 cp "$VERSION_FILE" "s3://$BUCKET_NAME/version.txt" --region "$REGION"

echo "==============================================="
echo " Deployment to S3 Successful!"
echo "==============================================="
