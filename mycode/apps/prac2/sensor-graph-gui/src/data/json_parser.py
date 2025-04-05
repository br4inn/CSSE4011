import json

def parse_sensor_data(json_string):
    try:
        data = json.loads(json_string)
        return data
    except json.JSONDecodeError:
        return None

def extract_sensor_values(data, dids):
    extracted_data = {}
    for did in dids:
        if did in data:
            extracted_data[did] = data[did]
    return extracted_data

def validate_sensor_data(data):
    required_keys = ['did', 'rtc_time', 'value']
    for key in required_keys:
        if key not in data:
            return False
    return True

def parse_and_validate(json_string, dids):
    data = parse_sensor_data(json_string)
    if data and validate_sensor_data(data):
        return extract_sensor_values(data, dids)
    return None