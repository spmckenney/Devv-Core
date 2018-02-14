#/bin/bash

WORKON_HOME=/home/bcrunner/py_virtual_env
VIRTUALENVWRAPPER_PYTHON=`which python3`

mkdir -p ${WORKON_HOME}
source /usr/local/bin/virtualenvwrapper.sh
mkvirtualenv --system-site-packages bc-dev
pip install --upgrade ipython

# Bug in pip/scipy, numpy must be installed first
pip install --upgrade numpy

pip install --upgrade \
    nose \
    nose2 \
    parameterized \
    grpcio \
    matplotlib \
    pillow \
    portpicker \
    scipy \
    xmlrunner
