from matplotlib import pyplot as plt
import numpy as np
import json

if __name__ == '__main__':
    from sys import argv

    if len(argv) < 2:
        print("Usage: python view_profiler.py <file_name>")
        exit(-1)

    file_name = argv[1]
    with open(file_name, 'r') as f:
        profile_data = json.loads(f.read())
        legends = []
        for k, v in profile_data['data'].items():
            plt.plot(np.arange(len(v)), np.array(v) / 1000)
            legends.append(k)
        plt.ylabel('Average Cost (ms)')
        plt.xlabel('Timeline')
        plt.title(file_name)
        plt.legend(legends)
        plt.show()
