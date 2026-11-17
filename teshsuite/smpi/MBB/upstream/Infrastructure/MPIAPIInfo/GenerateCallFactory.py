# THIS FILE IS NOT FOR PUBLICATION
# it is only used to generate the MPICallFactory code

import json


from Infrastructure.MPIAPIInfo.MPIAPIParameters import get_mpi_version_dict

template = """
    @staticmethod
    def @{FUNC_KEY}@(*args):
        return MPICall("@{FUNC_NAME}@", OrderedDict(@{PARAM_DICT}@), "@{VERSION}@")
"""

file_header="""#! /usr/bin/python3
from collections import OrderedDict

from Infrastructure.MPICall import MPICall

class MPICallFactory:

    @staticmethod
    def get(func: str, *args):
        f_to_call = getattr(MPICallFactory, func)
        return f_to_call(*args)

"""

correct_call_factory_header="""

from Infrastructure.CorrectParameter import CorrectParameterFactory

class CorrectMPICallFactory:

    @staticmethod
    def get(func: str):
        f_to_call = getattr(CorrectMPICallFactory, func)
        return f_to_call()

"""
template_correct = """
    @staticmethod
    def @{FUNC_KEY}@():
        correct_params = CorrectParameterFactory()
        return MPICallFactory().@{FUNC_KEY}@(@{PARAMS}@)
"""

def main():
    # read in the "official" standards json to get all mpi functions and there params
    mpi_api_json_file = "scripts/Infrastructure/MPIAPIInfo/MPI_api.json"
    output_file = "scripts/Infrastructure/MPICallFactory.py"

    with open(mpi_api_json_file, "r") as file:
        api_specs = json.load(file)


    class_str = file_header
    correct_class_str = correct_call_factory_header


    version_dict = get_mpi_version_dict()
    for key, api_spec in api_specs.items():
        spec = api_specs[key]
        name = spec['name']
        dict_str = "["
        correct_param_str = ""

        i = 0
        for param in spec['parameters']:
            if 'c_parameter' not in param['suppress']:
                dict_str = dict_str + "(\"" + param['name'] + "\", args[" + str(i) + "]),"
                correct_param_str = correct_param_str + "correct_params.get(\""+param['name']+"\"),"
                i = i + 1
                pass
        dict_str = dict_str + "]"
        correct_param_str=correct_param_str[:-1]# remove last ,

        ver = "4.0"
        # everyting not in dict is 4.0
        if (name in version_dict):
            ver = version_dict[name]


        function_def_str = (template.replace("@{FUNC_KEY}@", key)
                            .replace("@{FUNC_NAME}@", name)
                            .replace("@{PARAM_DICT}@", dict_str)
                            .replace("@{VERSION}@", ver))
        class_str = class_str+ function_def_str

        correct_function_def_str =(template_correct
                                   .replace("@{FUNC_KEY}@", key)
                                   .replace("@{PARAMS}@", correct_param_str))
        correct_class_str=correct_class_str+ correct_function_def_str

    with open(output_file,"w") as outfile:
        outfile.write(class_str+correct_class_str)

if __name__ == "__main__":
    main()
