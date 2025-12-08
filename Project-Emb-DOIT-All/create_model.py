import numpy as np
import pickle

np.random.seed(42)

# Mock 100 rows: feature <= 25 -> target = 1, feature > 25 -> target = 0
features = np.random.randint(1, 100, 100)
targets = (features <= 25).astype(int)

X = features.reshape(-1, 1)
y = targets

print("Sample data:")
for i in range(10):
    print(f"  feature: {X[i][0]}, target: {y[i]}")
print(f"...\nTotal: {len(X)} rows")
print(f"Target 1: {sum(y)}, Target 0: {len(y) - sum(y)}")

# Train with sklearn GradientBoosting (LightGBM alternative)
from sklearn.ensemble import GradientBoostingClassifier

model = GradientBoostingClassifier(n_estimators=100, learning_rate=0.1, max_depth=3)
model.fit(X, y)

# Test
test_cases = [25, 26, 10, 50, 24, 30]
print("\nTest predictions:")
for val in test_cases:
    pred = model.predict([[val]])[0]
    prob = model.predict_proba([[val]])[0][1]
    print(f"  feature={val} -> prediction={pred}, prob={prob:.3f}")

# Save model
with open("motor_prediction_model.pkl", "wb") as f:
    pickle.dump(model, f)

print("\nModel saved: motor_prediction_model.pkl")

