# Rearrangement

Instructions for how to use this project can be found in the text files RequestGuide.txt and TurnOnGuide.txt in the repository.

## Build Server

```bash
pip install -r requirements.txt
cd PythonRearrangement
python setup.py build_ext --inplace
```

## Start Server
Use set instead of export if you are on Windows.
Host is only necessary if you need access from another machine.

```bash
export FLASK_APP=rearrangementServer.py && flask run --host=0.0.0.0
```
