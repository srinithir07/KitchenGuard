import pandas as pd
import numpy as np
from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import train_test_split
from sklearn.metrics import classification_report, accuracy_score
from sklearn.preprocessing import LabelEncoder
import pickle
import os

def engineer_features(df):
    df["temp_gas_index"] = df["temperature"] * df["gas"] / 1000
    df["danger_score"] = (
        (df["temperature"] > 60).astype(int) +
        (df["gas"] > 1000).astype(int) +
        (df["motion"] == 0).astype(int) +
        (df["flame"] == 0).astype(int)
    )
    return df

def train():
    # Load data
    csv_path = "ml/data.csv"
    if not os.path.exists(csv_path):
        print("data.csv not found! Run generate_data.py first")
        return

    df = pd.read_csv(csv_path)
    df = engineer_features(df)
    print(f"Loaded {len(df)} samples")

    FEATURES = [
        "temperature", "humidity", "gas",
        "motion", "flame",
        "temp_gas_index", "danger_score"
    ]

    X = df[FEATURES]
    y = df["label"]

    # Encode labels
    le = LabelEncoder()
    y_encoded = le.fit_transform(y)

    # Split
    X_train, X_test, y_train, y_test = train_test_split(
        X, y_encoded,
        test_size=0.2,
        random_state=42,
        stratify=y_encoded
    )

    # Train Random Forest
    model = RandomForestClassifier(
        n_estimators=100,
        max_depth=10,
        random_state=42,
        n_jobs=-1
    )
    model.fit(X_train, y_train)

    # Evaluate
    y_pred = model.predict(X_test)
    print(f"\nAccuracy: {accuracy_score(y_test, y_pred)*100:.2f}%\n")
    print(classification_report(
        y_test, y_pred,
        target_names=le.classes_
    ))

    # Save
    os.makedirs("model", exist_ok=True)
    with open("model/model.pkl", "wb") as f:
        pickle.dump(model, f)
    with open("model/label_encoder.pkl", "wb") as f:
        pickle.dump(le, f)

    print("Model saved to model/model.pkl")

if __name__ == "__main__":
    train()
