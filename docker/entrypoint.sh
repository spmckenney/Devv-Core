#!/bin/bash

# Modify dcrunner to have UID and GID of user
# Either use the LOCAL_USER_ID if passed in at runtime or
# fallback

H_USER_ID=${USER_ID:-1002}
H_GROUP_ID=${GROUP_ID:-1002}

echo "Changing ownership of /opt/dcrunner"
chown_cmd="chown -R $H_USER_ID:$H_GROUP_ID /opt/dcrunner"

sudo usermod -u $H_USER_ID dcrunner
sudo ${chown_cmd}
echo "Starting with UID:GID : $H_USER_ID:$H_GROUP_ID"

if [ "$#" -eq 0 ]; then
su dcrunner
else
sudo -H -u dcrunner -g dcrunner "$@"
fi
