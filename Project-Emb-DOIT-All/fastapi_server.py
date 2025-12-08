from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
import numpy as np
import pickle
import os

app = FastAPI(title="LightGBM Prediction API")
MODEL_PATH = "motor_prediction_model.pkl"
model = None


class PredictRequest(BaseModel):
    feature: float


class PredictResponse(BaseModel):
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
        raise Exception(f"Model not found: {MODEL_PATH}")


@app.on_event("startup")
async def startup():
    load_model()


@app.get("/")
async def root():
    return {"message": "LightGBM Prediction API", "status": "running"}


@app.get("/health")
async def health():
    return {"status": "healthy", "model_loaded": model is not None}


@app.post("/predict", response_model=PredictResponse)
async def predict(data: PredictRequest):
    if model is None:
        raise HTTPException(500, "Model not loaded")
    
    features = np.array([[data.feature]])
    pred = int(model.predict(features)[0])
    prob = float(model.predict_proba(features)[0][1])
    
    return PredictResponse(
        prediction=pred,
        probability=prob,
        message="Open" if pred == 1 else "Close"
    )


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
