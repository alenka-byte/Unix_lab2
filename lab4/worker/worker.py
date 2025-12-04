import redis
import json
import time

redis_client = redis.Redis(host='redis', port=6379, db=0)

print("Worker started. Waiting for tasks...")

while True:
    task_json = redis_client.blpop('addition_tasks', timeout=30)

    if task_json:
        task_data = json.loads(task_json[1])
        task_id = task_data['task_id']
        a = task_data['a']
        b = task_data['b']

        print(f"Processing task {task_id}: {a} + {b}")

        result = a + b

        redis_client.setex(
            f'result:{task_id}',
            300,
            json.dumps({'a': a, 'b': b, 'sum': result})
        )

        print(f"Task {task_id} completed: {result}")

    time.sleep(0.1)
