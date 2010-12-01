#!/bin/bash

ssh pipol pipol-sub esn i386_kvm-linux-debian-lenny none 02:00 "~/Experimental.sh"
ssh pipol pipol-sub esn i386_kvm-linux-debian-testing none 02:00 "~/Experimental.sh"

ssh pipol pipol-sub esn amd64_kvm-linux-debian-lenny none 02:00 "~/Experimental.sh"
ssh pipol pipol-sub esn amd64_kvm-linux-debian-testing none 02:00 "~/Experimental.sh"