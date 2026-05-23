import pandas as pd
import numpy as np

np.random.seed(42)
n = 1000

def generate_data():
    records = []

    # SAFE: Person present, normal cooking
    for _ in range(n):
        records.append({
            "temperature": np.random.uniform(25, 45),
            "humidity":    np.random.uniform(50, 80),
            "gas":         np.random.randint(200, 600),
            "motion":      1,
            "flame":       1,
            "label":       "SAFE"
        })

    # MODERATE: Person briefly away
    for _ in range(n):
        records.append({
            "temperature": np.random.uniform(45, 65),
            "humidity":    np.random.uniform(35, 60),
            "gas":         np.random.randint(500, 1200),
            "motion":      0,
            "flame":       1,
            "label":       "MODERATE"
        })

    # HIGH_RISK: No person, high temp, high gas
    for _ in range(n):
        records.append({
            "temperature": np.random.uniform(65, 85),
            "humidity":    np.random.uniform(20, 40),
            "gas":         np.random.randint(1200, 2500),
            "motion":      0,
            "flame":       1,
            "label":       "HIGH_RISK"
        })

    # CRITICAL: Fire + high gas + no person
    for _ in range(n):
        records.append({
            "temperature": np.random.uniform(80, 120),
            "humidity":    np.random.uniform(10, 25),
            "gas":         np.random.randint(2500, 4095),
            "motion":      0,
            "flame":       0,
            "label":       "CRITICAL"
        })

    df = pd.DataFrame(records)
    df.to_csv("ml/data.csv", index=False)
    print(f"Dataset generated: {len(df)} samples")
    print(df["label"].value_counts())
    return df

if __name__ == "__main__":
    generate_data()
