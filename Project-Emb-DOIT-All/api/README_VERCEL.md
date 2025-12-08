# FastAPI LightGBM Deploy on Vercel

โครงสร้างโปรเจกต์สำหรับ deploy FastAPI + LightGBM บน Vercel

## โครงสร้างไฟล์

```
Project-Emb-DOIT-All/
├── api/
│   ├── index.py                    # FastAPI application
│   └── motor_prediction_model.pkl   # ML model
├── requirements.txt                 # Python dependencies
├── vercel.json                      # Vercel configuration
└── .gitignore                       # Git ignore rules
```

## วิธี Deploy บน Vercel

### 1. เตรียมโค้ด
- โครงสร้างไฟล์พร้อมแล้ว
- Model อยู่ใน `api/motor_prediction_model.pkl`

### 2. Push ขึ้น GitHub
```bash
git init
git add .
git commit -m "Initial commit: FastAPI + LightGBM"
git remote add origin <your-github-repo-url>
git push -u origin main
```

### 3. Deploy บน Vercel
1. ไปที่ [vercel.com](https://vercel.com)
2. Login with GitHub
3. กด "Add New..." → "Project"
4. Import repository ที่เพิ่ง push
5. กด Deploy (ไม่ต้องตั้งค่าเพิ่ม)

### 4. ทดสอบ API

หลังจาก deploy เสร็จ จะได้ URL เช่น: `https://your-project.vercel.app`

**ทดสอบ:**
```bash
# Test root endpoint
curl https://your-project.vercel.app/

# Test prediction
curl -X POST https://your-project.vercel.app/predict \
  -H "Content-Type: application/json" \
  -d '{"feature": 25}'
```

**Response:**
```json
{
  "prediction": 1,
  "probability": 0.999,
  "message": "Open"
}
```

## Model Logic
- `feature <= 25` → prediction = 1 (Open)
- `feature > 25` → prediction = 0 (Close)

