import csv
from typing import List
import sklearn
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

INPUT_FILE = "output.csv"


class DataRow():
    """
    A row of data within the CSV file
    """
    def __init__(self, access: str, nanoseconds: int):
        self.access = access
        self.nanoseconds = nanoseconds
    
    def __repr__(self):
        return self.access + "," + str(self.nanoseconds)
 
def get_csv_data():
    """
    Gather CSV data from input file
    """
    output = []
    with open(INPUT_FILE, newline='') as f:
        reader = csv.reader(f)

        # Skip first line
        next(reader)
        for row in reader:
            output.append(DataRow(row[0], int(row[1])))

    return output

def group_data_into_L1_L2_and_DRAM(data: List[DataRow]) -> pd.DataFrame:
    """
    Gather data into 3 groups to represent L1, L2, and DRAM
    using K Means
    
    :param data: The data to group
    :type data: List[DataRow]
    :return: A Data frame of the formatted data
    :rtype: pd.DataFrame
    """
    formatted_data = [{'nanoseconds': val.nanoseconds} for i, val in enumerate(data)]
    df = pd.DataFrame(formatted_data)

    # 1. Log Transform the data first
    df['log_ns'] = np.log1p(df['nanoseconds'])

    # 2. Run KMeans on the LOGged values
    data_reshaped = df['log_ns'].values.reshape(-1, 1)
    kmeans = sklearn.cluster.KMeans(n_clusters=3, random_state=0).fit(data_reshaped)

    # 3. Assign labels
    df['cluster_id'] = kmeans.labels_

    # 4. Map to L1/L2/DRAM based on the log-mean
    cluster_means = df.groupby('cluster_id')['log_ns'].mean().sort_values()
    label_map = {
        cluster_means.index[0]: 'L1',
        cluster_means.index[1]: 'L2',
        cluster_means.index[2]: 'DRAM',
    }

    df['category'] = df['cluster_id'].map(label_map)

    return df

def main():

    # Get and group the data into K Means where K is 3 (For each level)
    data = get_csv_data()
    data = group_data_into_L1_L2_and_DRAM(data)

    # Create scatterplot of memory accesses
    color_map = {'L1': 'red', 'L2': 'orange', 'DRAM': 'yellow'}
    plt.figure(figsize=(10, 6))
    for category, color in color_map.items():
        subset = data[data['category'] == category]
        plt.scatter(
            subset.index, 
            subset['nanoseconds'], 
            c=color, 
            label=category, 
            alpha=0.6,
            s=20
        )

    plt.title('Memory Access Latency Clustered by K-Means')
    plt.xlabel('Data Index') # Array of memory accessed
    plt.ylabel('Access Time (Nanoseconds)')
    plt.yscale('log') # Turn into base 10 since memory access starts with ~1200
    plt.legend(title='Memory Level')
    plt.grid(True, which="both", ls="--")
    plt.savefig('memory-scatterplot.png')
    
if __name__ == "__main__":
    main()