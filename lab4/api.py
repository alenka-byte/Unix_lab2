from flask import Flask, request, jsonify
import redis
import uuid
import json

app = Flask(__name__)
redis_client = redis.Redis(host='redis', port=6379, db=0)

@app.route('/add', methods=['POST'])
def add_numbers():
    data = request.get_json()
    a = data.get('a')
    b = data.get('b')

    if a is None or b is None:
        return jsonify({'error': 'Missing parameters'}), 400

    task_id = str(uuid.uuid4())
    task_data = {
        'task_id': task_id,
        'a': a,
        'b': b
    }

    redis_client.rpush('addition_tasks', json.dumps(task_data))

    return jsonify({'task_id': task_id, 'status': 'queued'}), 202

@app.route('/result/<task_id>', methods=['GET'])
def get_result(task_id):
    result = redis_client.get(f'result:{task_id}')
    if result:
        return jsonify({'task_id': task_id, 'result': json.loads(result)})
    return jsonify({'task_id': task_id, 'status': 'processing'}), 202

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
