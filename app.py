from flask import Flask, request, jsonify
import pickle
import numpy as np
import logging
import os

app = Flask(__name__)
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Load model
try:
    with open("model/model.pkl", "rb") as f:
        model = pickle.load(f)
    with open("model/label_encoder.pkl", "rb") as f:
        label_encoder = pickle.load(f)
    logger.info("✅ Model loaded!")
except Exception as e:
    logger.error(f"Model load failed: {e}")
    model = None
    label_encoder = None

def engineer_features(data):
    temp   = float(data["temperature"])
    gas    = float(data["gas"])
    motion = float(data["motion"])
    flame  = float(data["flame"])
    temp_gas_index = temp * gas / 1000
    danger_score = (
        int(temp > 60) +
        int(gas > 1000) +
        int(motion == 0) +
        int(flame == 0)
    )
    return temp_gas_index, danger_score

def fallback_predict(data):
    gas    = float(data["gas"])
    temp   = float(data["temperature"])
    motion = float(data["motion"])
    flame  = float(data["flame"])
    if flame == 0 and gas > 2500: return "CRITICAL"
    if temp > 65 and gas > 1200:  return "HIGH_RISK"
    if motion == 0 and gas > 800: return "MODERATE"
    return "SAFE"

@app.route("/predict", methods=["POST"])
def predict():
    try:
        data = request.get_json(force=True)
        logger.info(f"Received: {data}")

        if model is None:
            return fallback_predict(data), 200

        temp_gas_index, danger_score = engineer_features(data)

        features = np.array([[
            float(data["temperature"]),
            float(data["humidity"]),
            float(data["gas"]),
            float(data["motion"]),
            float(data["flame"]),
            temp_gas_index,
            danger_score
        ]])

        pred  = model.predict(features)[0]
        risk  = label_encoder.inverse_transform([pred])[0]
        logger.info(f"→ Predicted: {risk}")
        return risk, 200

    except Exception as e:
        logger.error(f"Error: {e}")
        return "SAFE", 500

@app.route("/health", methods=["GET"])
def health():
    return jsonify({"status": "online"}), 200

if __name__ == "__main__":
    print("\n" + "="*40)
    print("  KitchenGuard ML Server Running!")
    print("  Listening on: 0.0.0.0:5000")
    print("="*40 + "\n")
    app.run(host="0.0.0.0", port=5000, debug=False)