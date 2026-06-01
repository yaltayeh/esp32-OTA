#!/bin/bash

set -e

# إيقاف السكريبت فوراً في حال حدوث أي خطأ أثناء التنفيذ

# === [إعداد المتغيرات] ===
# استبدل هذا الاسم باسم الـ Bucket الذي أنشأته في الـ Terraform
BUCKET_NAME="esp32-ota-storage-bin-2026" 
# جلب المنطقة ديناميكياً من بيانات خادم EC2 (IMDSv2) أو استخدام القيمة الافتراضية
TOKEN=$(curl -s -X PUT "http://169.254.169.254/latest/api/token" -H "X-aws-ec2-metadata-token-ttl-seconds: 60" 2>/dev/null || true)
if [ -n "$TOKEN" ]; then
    REGION=$(curl -s -H "X-aws-ec2-metadata-token: $TOKEN" http://169.254.169.254/latest/meta-data/placement/region 2>/dev/null || echo "eu-north-1")
else
    REGION="eu-north-1"
fi

BUILD_DIR="build"
VERSION_FILE="$BUILD_DIR/version.bin"
FIRMWARE_FILE="$BUILD_DIR/test_OTA.bin"

echo "==============================================="
echo " Starting After-Build Deployment Process "
echo "==============================================="

# 1. استخراج رقم الإصدار وتوليد ملف version.bin
python3 ./scripts/update_version.py main/version.h $VERSION_FILE

# 2. التأكد من أن ملفات البناء موجودة وسليمة
echo "--> [2/3] Verifying build binaries..."
if [ ! -f "$FIRMWARE_FILE" ]; then
    echo "❌ Error: firmware.bin not found! Did the compilation fail?"
    exit 1
fi

# 3. الرفع إلى AWS S3 باستخدام AWS CLI
echo "--> [3/3] Uploading binaries to AWS S3..."

# رفع ملف السوفتوير الرئيسي (firmware.bin)
echo "Uploading firmware.bin..."
aws s3 cp "$FIRMWARE_FILE" "s3://$BUCKET_NAME/firmware.bin" --region "$REGION"

# رفع ملف رقم الإصدار (version.bin)
echo "Uploading version.bin..."
aws s3 cp "$VERSION_FILE" "s3://$BUCKET_NAME/version.bin" --region "$REGION"

echo "==============================================="
echo " ✅ Deployment to S3 Successful!"
echo "==============================================="