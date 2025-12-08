from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
import numpy as np
import pickle
import os

app = FastAPI()

current_dir = os.path.dirname(os.path.abspath(__file__))
MODEL_PATH = os.path.join(current_dir, "motor_prediction_model.pkl")
model = None

try:
    if os.path.exists(MODEL_PATH):
        with open(MODEL_PATH, 'rb') as f:
            model = pickle.load(f)
        print("Model loaded successfully.")
    else:
        print(f"Warning: Model file not found at {MODEL_PATH}")
except Exception as e:
    print(f"Error loading model: {e}")


class PredictRequest(BaseModel):
    feature: float


class PredictResponse(BaseModel):
    prediction: int
    probability: float
    message: str


@app.get("/")
async def root():
    return {"message": "API is running on Vercel!", "model_status": "loaded" if model else "failed"}


@app.post("/predict", response_model=PredictResponse)
async def predict(data: PredictRequest):
    if model is None:
        raise HTTPException(status_code=500, detail="Model is not loaded.")

    features = np.array([[data.feature]])
    
    pred = int(model.predict(features)[0])
    
    try:
        prob = float(model.predict_proba(features)[0][1])
    except:
        prob = 0.0

    return PredictResponse(
        prediction=pred,
        probability=prob,
        message="Open" if pred == 1 else "Close"
    )

