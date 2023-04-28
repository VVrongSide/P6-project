
###########################
# Import libraies
import csv
import numpy as np
import matplotlib.pyplot as plt

###########################
# VARIABLES
filename = "result.csv"

BASELINE = 0

# Timestamp(ms),    Current(uA),    D0-D7,  D0, D1, D2, D3, D4, D5, D6, D7

###########################
# FUNCTIONS

def master_info(filename):
    
    process_array = []      # Entries for processing data
    new_process = []
    process_count = 0
    process_state = False   # End device processing   

    energy_transmit = []    # Entries for transmitting data
    transmit_count = 0
    transmit_state = False  # End device transmitting

    samples = 0           # Total number of samples

    with open(filename,'r') as csvfile:
        reader_obj = csv.reader(csvfile)
        
        for row in reader_obj:

            # Isolate periods of data processing
            if row[3] == '1':
                if process_state == False: # Begining of a data process
                    new_process.append(row)    
                    process_state = True
                elif process_state == True:
                    new_process.append(row)
            elif row[3] == '0':
                if process_state == True:   # End of data process
                    process_array.append(new_process) 
                    new_process = []
                    process_state = False
            # Isolate periods of data transmission
                # bla bla

    return process_array


def session_analysis(array):
    
    total_energy = 0
    array_energy = []

    for session in array:
        for index, row in enumerate(session):
            if index == 0:
                start_time = float(row[0])
            elif index == len(session)-1:
                stop_time = float(row[0])
            
            total_energy += float(row[1])
        array_energy.append([total_energy, stop_time-start_time, len(session)])
        total_energy = 0

    return array_energy


def histogram_plots():
    # data_array[(),()]
    data_array = (2, 1.5, 1.8, 2.1, 2)
    transmit_array = (10, 11, 10.5, 11.5, 9.45)
    event_num = [i+1 for i in range(len(data_array))]

    fig = plt.figure(figsize=(6,5), dpi=200)
    left, bottom, width, height = 0.1, 0.1, 0.8, 0.8
    ax = fig.add_axes([left, bottom, width, height]) 
    
    width = 0.20   
    ticks = np.arange(len(event_num))    
    ax.bar(ticks, data_array, width, color="#264653", label='Data processing')
    ax.bar(ticks + width, transmit_array, width, color="#2A9D8F", align="center",
        label='Data transmission')

    ax.set_ylabel('Ampere $\mu A$')
    ax.set_xlabel('Events')
    ax.set_title('Energy consumption')
    ax.set_xticks(ticks + width/2)
    ax.set_xticklabels(event_num)

    ax.legend(loc='upper right')
    plt.ylim(0, max(transmit_array)+5)
    plt.show()


###########################
# MAIN
if __name__ == "__main__":

    # Isolate areas for data processing and transmission
    #process_array = master_info(filename)

    # Analysis individuel events
    #process_results = session_analysis(process_array)
    #print(process_results)
    histogram_plots()
