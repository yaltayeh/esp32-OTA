#!/bin/bash

set -e

# إيقاف السكريبت فوراً في حال حدوث أي خطأ أثناء التنفيذ

# === [إعداد المتغيرات] ===
# استبدل هذا الاسم باسم الـ Bucket الذي أنشأته في الـ Terraform
BUCKET_NAME="esp32-ota-storage-bin-2026" 
REGION="eu-north-1" # المنطقة الخاصة بالسيرفر والـ S3

BUILD_DIR="../build"
VERSION_FILE="$BUILD_DIR/version.bin"
FIRMWARE_FILE="$BUILD_DIR/firmware.bin"

echo "==============================================="
echo " Starting After-Build Deployment Process "
echo "==============================================="

# 1. استخراج رقم الإصدار وتوليد ملف version.bin
python3 ./update_version.py ../main/version.h $VERSION_FILE

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