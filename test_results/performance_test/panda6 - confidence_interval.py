import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import datetime
from scipy.stats import t
import inspect

# ---------------------------------------------------------------------------------
def clean_and_isolate(files):
    
    # iterate over the list of file names and read each file into a DataFrame
    for index, csv_file in enumerate(files):
        df = pd.read_csv(csv_file)
    
    # Subtract the baseline
        df['Current(uA)'] = df['Current(uA)'].astype(float)
        num_negative = (df['Current(uA)'] < 0).sum()
    
    # Removes unnecessary columns
        drop_list = ['D0-D7','D3','D4','D5','D6','D7']
        for column in drop_list:
            df.pop(column)

        # Put cleaned file back
            files[index] = df
    return files

# ---------------------------------------------------------------------------------
def segmentation(filenames):

    segments_array = []
    for df in filenames:
        
        # read the csv file into a Dataframe
        df = pd.read_csv(df)
    
        # find the indices where the segmentation occurs
        segment_indices = df.index[df['D2'].diff() == -1].tolist()
        
        # add the first and last indices to create the segment boundaries
        segment_indices = [0] + segment_indices + [len(df)-1]

        # create a list of dataframes for each segment
        segments = [df.iloc[segment_indices[i]:segment_indices[i+1]] for i in range(len(segment_indices)-1)]
        
        # append list of dataframes to array/list
        segments_array.append(segments)

    return segments_array

# ---------------------------------------------------------------------------------
def number_crunching(files):

    time = []
    energy = []

    for index, data in enumerate(files):
        
        # removes the last segement in each file
        data.pop(-1)
        print('\n###### {} ######'.format(filenames[index]))

        process_joule_array = []
        process_time_array = []

        transmit_joule_array = []
        transmit_time_array = []

        receive_joule_array = []
        receive_time_array = []

        total_joule_array = []
        total_time_array = []

        # enters a csv file
        for index, df in enumerate(data):
            
            # calculate the average baseline of the 'Current(uA)'        
            baseline_current = df.loc[(df['D0'] == 0) & (df['D1'] == 0) & (df['D2'] == 0), 'Current(uA)'].mean()
            
            # subtract baseline from ALL rows
            df['Current(uA)'] = df['Current(uA)'].astype(float) -  baseline_current - pin_energy
            
            # ---------------------------------------------------------------------------------            
            ### PROCESS

            # Isolate all process rows in one dataframe
            process = df[df['D0'] == 1]

            # --- Time
            # Time used for each process 
            start_time = float(process['Timestamp(ms)'].iloc[0])
            end_time = float(process['Timestamp(ms)'].iloc[-1])
            process_time = end_time - start_time
            process_time_sec = process_time/1000     # ms / 1.000 = seconds
            
            # append process time to array
            process_time_array.append(process_time_sec)

            # --- Energy
            # mean of ampere used for each process
            process_mean = process['Current(uA)'].mean()

            # convert micro amps(uA) to amps(A) and multiply with voltage(V)
            process_watts = (process_mean/1000000)*5          # uA / 1.000.000 * I
            
            # calculate energy(joules) used in the process time
            process_joules = process_watts * process_time  #    W * Sec = joules
            
            #append process energy to array
            process_joule_array.append(process_joules)

            # ---------------------------------------------------------------------------------            
            ### TRANSMIT
            # Isolate all transmit rows in one dataframe
            transmit = df[df['D1'] == 1]
            
            # --- Time
            # Time used for each transmit 
            start_time = float(transmit['Timestamp(ms)'].iloc[0])
            end_time = float(transmit['Timestamp(ms)'].iloc[-1])
            transmit_time = end_time - start_time
            transmit_time_sec = transmit_time/1000     # ms / 1.000 = seconds
            
            # append transmit time to array
            transmit_time_array.append(transmit_time_sec)

            # --- Energy
            # mean of ampere used for each transmit
            transmit_mean = transmit['Current(uA)'].mean()

            # convert micro amps(uA) to amps(A) and multiply with voltage(V)
            transmit_watts = (transmit_mean/1000000)*5          # uA / 1.000.000 * I
            
            # calculate energy(joules) used in the transmit time
            transmit_joules = transmit_watts * transmit_time  #    W * Sec = joules
            
            #append transmit energy to array
            transmit_joule_array.append(transmit_joules)



            # ---------------------------------------------------------------------------------            
            ### Total
            total_joule_array.append(process_joules+transmit_joules)
            total_time_array.append(process_time_sec+transmit_time_sec)
        time.append([process_time_array, transmit_time_array, total_time_array])
        energy.append([process_joule_array,transmit_joule_array,total_joule_array])
        
    return time, energy  

# ---------------------------------------------------------------------------------
def info(csv_files):
    for index, df in enumerate(csv_files):

        # read the csv file into a Dataframe
        data = pd.read_csv(df)

        data['start'] = (data['D0'] == 1) & (data['D0'].shift(1) == 0)
        
        data['series'] = data['start'].cumsum()
        
        num_periods = data[data['D0'] == 1]['series'].nunique()        
  
        print('File: {}\t Periods: {}'.format(filenames[index], num_periods))

def confidence_plot(array, plot):
    
    process_mean = []
    transmit_mean = []
    total_mean = []

    process_moe = []
    transmit_moe = []
    total_moe = []

    for byte in array:
        # calculate mean for mean for process, transmit and total
        process_mean.append(np.mean(byte[0]))
        transmit_mean.append(np.mean(byte[1]))
        total_mean.append(np.mean(byte[2]))


        # Calculate standard deviation of process, transmit and total    
            # set the desired confidence level (e.g., 95%)
        conf_level = 0.95
            # Calculate the corresponding t-value for the confidence level and degrees of freedom (n-1)
        t_value = t.ppf((1 + conf_level) / 2, df=len(byte))
            # calculate standard deviation
        process_std = np.std(byte[0])
        transmit_std = np.std(byte[1])
        total_std = np.std(byte[2])
            # calculate the margin of error (MOE)
        process_moe.append(t_value * process_std)
        transmit_moe.append(t_value * transmit_std)
        total_moe.append(t_value * total_std)
    
    print("")
    print('Process mean: {}'.format(process_mean))
    print('Transmit mean: {}'.format(transmit_mean))
    print('Total mean: {}'.format(total_mean))
    print("")
    print('Process MOE: {}'.format(process_moe))
    print('Transmit MOE: {}'.format(transmit_moe))
    print('Total MOE: {}'.format(total_moe))
    print("")

    # Create a plot with dots showing the mean and error bars representing the confidence interval
    fig, ax = plt.subplots()
    
    rects1 = ax.errorbar([2, 4, 8, 16], process_mean, yerr=process_moe, fmt='o', label='Process', color='#264653')
    rects2 = ax.errorbar([2, 4, 8, 16], transmit_mean, yerr=process_moe, fmt='o', label='Transmit', color='#2A9D8F')
    rects3 = ax.errorbar([2, 4, 8, 16], total_mean, yerr=process_moe, fmt='o', label='Total', color='#E9C46A')

    # Add labels, title, and legend
    ax.set_xticks(np.array([2,4,8,16]))
    ax.set_xticklabels(['2', '4', '8', '16'])
    ax.set_xlabel('Payload size(bytes)')

    if plot == 'energy':
        ax.set_ylabel('Mean energy(joules)')
        #ax.set_yscale('log')
    elif plot == 'time':
        ax.set_ylabel('Mean time(seconds)')
        
    #ax.set_title('Confidence Intervals with encryption of payload')
    ax.legend()

    plotname = plot + '.png'

    fig.savefig(plotname)
    

# ---------------------------------------------------------------------------------
# ---------------------------------------------------------------------------------
if __name__ == "__main__":
 
    pin_energy = 326 
    file_num = 0
    # Path to dirtory for csv files
    #dir_path = "/home/wrongside/OneDrive/Documents/ComTek/semester06/P6-project/test_and_results"
    
    #dir_path = "/home/wrongside/test_folder/plain_text"
    dir_path = "/home/wrongside/test_folder/encrypt_180sec"



    # Retreive a list of csv file
    #csv_files = [f for f in os.listdir(dir_path) if f.endswith('.csv')]
    csv_files = ['byte2.csv','byte4.csv','byte8.csv','byte16.csv']
    print('\nImported files: {}\n'.format(csv_files))
    filenames = []
    for file in csv_files:
        filenames.append(str(file))

    # State the number of periods in each csv file
    info(csv_files)

    # Cut every file into segments of a period and pervious baseline
    segmented_files = segmentation(filenames)

    time, energy = number_crunching(segmented_files)

    confidence_plot(energy, 'energy')
    confidence_plot(time, 'time')
    
    
    
