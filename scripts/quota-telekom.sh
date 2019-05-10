#!/bin/bash
curl --user-agent 'Mozilla/4.0' http://pass.telekom.de/api/service/generic/v1/status | jq .usedVolumeStr
