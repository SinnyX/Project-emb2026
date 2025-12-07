from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
import numpy as np
import pickle
import os
from typing import Optional

try:
    import lightgbm as lgb
    LIGHTGBM_AVAILABLE = True
except (ImportError, OSError):
    LIGHTGBM_AVAILABLE = False

app = FastAPI(title="ESP32 Motor Prediction API")
MODEL_PATH = "motor_prediction_model.pkl"
model = None


class SensorData(BaseModel):
    ultrasonic: float
    microphone: float
    humidity: Optional[float] = None
    temperature: Optional[float] = None
    photo: Optional[float] = None
    flame: Optional[float] = None


class PredictionResponse(BaseModel):
    prediction: int
    probability: float
    message: str


def load_model():
    global model
    if os.path.exists(MODEL_PATH):
        with open(MODEL_PATH, 'rb') as f:
            model = pickle.load(f)
        print(f"Model loaded: {MODEL_PATH}")
    else:
        create_model()


def create_model():
    global model
    np.random.seed(42)
    n = 2000
    
    ultrasonic = np.random.uniform(0, 100, n)
    microphone = np.random.uniform(0, 4095, n)
    
    # Label: Open(1) when ultra < 30 AND micro > 2000
    y = ((ultrasonic < 30) & (microphone > 2000)).astype(int)
    X = np.column_stack([ultrasonic, microphone])
    
    if LIGHTGBM_AVAILABLE:
        train_data = lgb.Dataset(X, label=y)
        params = {
            'objective': 'binary',
            'metric': 'binary_logloss',
            'boosting_type': 'gbdt',
            'num_leaves': 31,
            'learning_rate': 0.05,
            'verbose': -1
        }
        model = lgb.train(params, train_data, num_boost_round=100)
    else:
        from sklearn.ensemble import GradientBoostingClassifier
        model = GradientBoostingClassifier(n_estimators=100, learning_rate=0.05, max_depth=5)
        model.fit(X, y)
    
    with open(MODEL_PATH, 'wb') as f:
        pickle.dump(model, f)
    print(f"Model created: {MODEL_PATH}")


def predict(features: np.ndarray):
    if hasattr(model, 'predict_proba'):
        prob = float(model.predict_proba(features)[0][1])
        pred = int(model.predict(features)[0])
    else:
        prob = float(model.predict(features)[0])
        pred = 1 if prob > 0.5 else 0
    return pred, prob


@app.on_event("startup")
async def startup():
    load_model()


@app.get("/")
async def root():
    return {"message": "ESP32 Motor Prediction API", "status": "running"}


@app.get("/health")
async def health():
    return {"status": "healthy", "model_loaded": model is not None}


@app.post("/predict", response_model=PredictionResponse)
async def predict_motor(data: SensorData):
    if model is None:
        raise HTTPException(500, "Model not loaded")
    
    features = np.array([[data.ultrasonic, data.microphone]])
    pred, prob = predict(features)
    
    return PredictionResponse(
        prediction=pred,
        probability=prob,
        message="Open" if pred == 1 else "Close"
    )


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
