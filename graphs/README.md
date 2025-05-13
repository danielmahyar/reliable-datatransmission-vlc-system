# Graphs and Test results for Report
This directory contains all the test results and Python code for the graphs used in the report.


## CSV structure
Here is an example of how the general CSV structure looks like:

1) **Time**: The timestamp in seconds that the data row is sampled at
2) **Total Bits**: The total bits received so far
3) **Total Errors**: The total errors at the specific row
4) **BER**: Bit error rate using the errors at a specific row and compares it to the whole bitstream length. Mainly used for debugging dataset and not used in the graphs
5) **Bitrate**: The bitrate at the specific point in time the data was received

> Please note that the column *Total Errors* does account for the total amount of errors during the whole transmission, but only the errors for the received byte. Therefore the *BER* column is based on the specific byte errors to the whole transmission length


Here is an example of the data compiled during the experiments:

|Time|Total Bits|Total Errors|BER|Bitrate          |
|----|----------|------------|---|-----------------|
|0.009|8         |4           |0.004|888.8|
|0.012|16        |6           |0.006|1333.3|
|0.015|24        |5           |0.005|1600             |
|0.017|32        |2           |0.002|1882.4|
|0.02|40        |3           |0.003|2000             |
|0.023|48        |2           |0.002|2087|
|0.026|56        |5           |0.005|2153.8|
|0.028|64        |4           |0.004|2285.7|
|0.031|72        |5           |0.005|2322.6|
|0.034|80        |4           |0.004|2352.9|
|0.037|88        |7           |0.007|2378.4|
|0.039|96        |5           |0.005|2461.4|
|0.042|104       |4           |0.004|2476.2|


