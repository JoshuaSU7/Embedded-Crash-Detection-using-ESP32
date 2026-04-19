from flask import Flask, request, jsonify

app = Flask(__name__)

@app.route("/crash", methods=["POST"])
def crash():
    data = request.get_json()
    col_llh = data.get("col_llh", 0)
    print(f"CRASH DETECTED — confidence score: {col_llh:.2f}")
    return jsonify({"status": "ok"}), 200

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=80, debug=True)