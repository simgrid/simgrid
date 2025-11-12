import base64
import os
import cs
import sys


def main(cloudstack, action):
    ## It is a good practice to have the "{project name}-" prefix
    ## in VM names.
    vm_name = "simgrid-debian-testing-64-dynamic-analysis-2"
    try:
        vm = get_vm(cloudstack, vm_name)
    except KeyError:
        vm = None
    if action == "start":
        if vm is None:
            vm = create_vm(cloudstack, vm_name)
        if vm["state"] == "Stopped":
            start_vm(cloudstack, vm["id"])
    elif action == "stop":
        if vm is None:
            return
        if vm["state"] == "Running":
            stop_vm(cloudstack, vm["id"])
    elif action == "destroy":
        if vm is None:
            return
        destroy_vm(cloudstack, vm["id"])


def get_vm(cloudstack, vm_name):
    response = cloudstack.listVirtualMachines(name=vm_name)
    return response["virtualmachine"][0]


def create_vm(cloudstack, vm_name):
    userdata = f"""
#cloud-config
runcmd:
  - curl -L "https://packages.gitlab.com/install/repositories/runner/gitlab-runner/script.deb.sh" | sudo bash
  - apt-get install --yes gitlab-runner docker.io
  - |
    gitlab-runner register --non-interactive --tag-list debian-testing,docker \
      --executor docker --docker-image alpine --url https://gitlab.inria.fr \
      --registration-token {os.environ["REGISTRATION_TOKEN"]}
"""
    response = cloudstack.deployVirtualMachine(
        serviceOfferingId=get_service_offering_id(cloudstack, "Custom"),
        templateId=get_template_id(cloudstack, "featured", "ubuntu-20.04-lts"),
        zoneId=get_zone_id(cloudstack, "zone-ci"),
        name=vm_name,
        displayName=vm_name,
        details={"cpuNumber": 16, "memory": 4096},
        userdata=base64.b64encode(bytes(userdata, encoding="utf8")),
        fetch_result=True,
    )
    return response["virtualmachine"]


def start_vm(cloudstack, vm_id):
    cloudstack.startVirtualMachine(id=vm_id, fetch_result=True)


def stop_vm(cloudstack, vm_id):
    cloudstack.stopVirtualMachine(id=vm_id, fetch_result=True)


def destroy_vm(cloudstack, vm_id):
    cloudstack.destroyVirtualMachine(id=vm_id, expunge=True, fetch_result=True)


def get_service_offering_id(cloudstack, name):
    response = cloudstack.listServiceOfferings(name=name)
    return response["serviceoffering"][0]["id"]


def get_template_id(cloudstack, templatefilter, name):
    response = cloudstack.listTemplates(templatefilter=templatefilter, name=name)
    return response["template"][0]["id"]


def get_zone_id(cloudstack, name):
    response = cloudstack.listZones(name=name)
    return response["zone"][0]["id"]


cloudstack = cs.CloudStack(
    endpoint="https://sesi-cloud-ctl1.inria.fr/client/api/",
    key=os.environ["CLOUDSTACK_API_KEY"],
    secret=os.environ["CLOUDSTACK_SECRET_KEY"],
)

main(cloudstack, sys.argv[1])

