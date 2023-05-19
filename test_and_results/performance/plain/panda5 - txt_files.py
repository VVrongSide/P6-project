import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import datetime

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
            
            """
            print('Process time:\t {} ms'.format(process_time)) # milliseconds
            print('Process time:\t {} sec'.format(process_time_sec)) # seconds 
            print('Process mean:\t {} uA'.format(process_mean))
            print('Process watt:\t {} W'.format(process_watts)) # Watts per second
            print(('Energy:\t {} joules'.format(process_joules)))
            print("")
            """
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
            ### RECEIVE
             # Isolate all receive rows in one dataframe
            receive = df[df['D2'] == 1]
            
            # --- Time
            # Time used for each receive 
            start_time = float(receive['Timestamp(ms)'].iloc[0])
            end_time = float(receive['Timestamp(ms)'].iloc[-1])
            receive_time = end_time - start_time
            receive_time_sec = receive_time/1000     # ms / 1.000 = seconds
            
            # append receive time to array
            receive_time_array.append(receive_time_sec)

            # --- Energy
            # mean of ampere used for each receive
            receive_mean = receive['Current(uA)'].mean()

            # convert micro amps(uA) to amps(A) and multiply with voltage(V)
            receive_watts = (receive_mean/1000000)*5          # uA / 1.000.000 * I
            
            # calculate energy(joules) used in the receive time
            receive_joules = receive_watts * receive_time  #    W * Sec = joules
            
            #append receive energy to array
            receive_joule_array.append(receive_joules)

            # ---------------------------------------------------------------------------------            
            ### Total
            total_joule_array.append(process_joules+transmit_joules+receive_joules)
            total_time_array.append(process_time_sec+transmit_time_sec+receive_time_sec)

        # plot the energy and time per period for this csv file    
        #plots(process_joule_array, transmit_joule_array, receive_joule_array, total_joule_array, process_time_array, transmit_time_array, receive_time_array, total_time_array)    
        
        # wirte time and energy result for each period to txt file
        write_out(process_joule_array, transmit_joule_array, receive_joule_array, total_joule_array, process_time_array, transmit_time_array, receive_time_array, total_time_array)

# ---------------------------------------------------------------------------------
def mean_variance(array):
    array_length = len(array)
    mean = sum(array) / array_length

    
    squared_differences = []
    for index, value in enumerate(array):
        new_value = (value - mean)**2
        squared_differences.append(new_value)

    sum_squared_differences = sum(squared_differences)

    variance = sum_squared_differences / (array_length-1)

    return mean, variance

        
# ---------------------------------------------------------------------------------
def write_out(process_energy, transmit_energy, receive_energy, total_energy, process_time, transmit_time, receive_time, total_time):
    global file_num
    
    # --- STATISTIC ANALYSIS OF MEASUREMENT
    # ---------------------------------------------------------------------------------
    # Calculate the mean of the process, transmit, receive and total in time and energy
    
    #print(process_time)
    #print(transmit_time)
    #print(receive_time)
    #print(total_time)

    process_energy_mean, process_energy_variance  = mean_variance(process_energy)
    transmit_energy_mean, transmit_energy_variance = mean_variance(transmit_energy)
    receive_energy_mean, receive_energy_variance = mean_variance(receive_energy)
    total_energy_mean, total_energy_variance = mean_variance(total_energy)
    
    process_time_mean, process_time_variance = mean_variance(process_time)
    transmit_time_mean, transmit_time_variance = mean_variance(transmit_time)
    receive_time_mean, receive_time_variance = mean_variance(receive_time)
    total_time_mean, total_time_variance = mean_variance(total_time)

    # Calculate the variance of the process, transmit, receive and total in time and energy
     #= statistics.variance(process_energy, process_energy_mean)
     #= statistics.variance(transmit_energy, transmit_energy_mean)
     #= statistics.variance(receive_energy, receive_energy_mean)
     #= statistics.variance(total_energy, total_energy_mean)

    #print(process_time)
    #print(process_energy_mean)

    # = statistics.variance(process_time, process_time_mean)
    # = statistics.variance(transmit_time, transmit_time_mean)
    # = statistics.variance(receive_time, receive_time_mean)
    # = statistics.variance(total_time, total_time_mean)


    # ---------------------------------------------------------------------------------
    # WRITE TO FILE
    # get name of current csv file
    txt_name = filenames[file_num]
    txt_name = txt_name.replace('.csv', '.txt')
    file_num += 1
    
    # get the current date and time
    now = datetime.datetime.now()

    # format the date and time as a string
    formatted = now.strftime("%d-%m-%Y %H:%M:%S")

    periods = len(total_energy)

    # open file for writting
    with open(txt_name, 'w') as f:
        f.write('################## {} ##################\n'.format(txt_name))
        f.write('\nTime:\t\t {}\n'.format(formatted))
        f.write('Periods:\t {}\n\n'.format(len(total_energy)))
        f.write('################# PERIODS #################\n')
        
        for num in range(periods):
            f.write('###### Period no. {} ######\n'.format(num+1))
            f.write('--- Time ---\n')
            f.write('Process time:\t {} sec\n'.format(format(process_time[num],".6f")))
            f.write('Transmit time:\t {} sec\n'.format(format(transmit_time[num],".6f")))
            f.write('Receive time:\t {} sec\n'.format(format(receive_time[num],".6f")))
            f.write('Total time:\t\t {} sec\n'.format(format(total_time[num],".6f")))
            f.write('--- Energy ---\n')
            f.write('Process energy:\t\t {} joules\n'.format(format(process_energy[num],".6f")))
            f.write('Transmit energy:\t {} joules\n'.format(format(transmit_energy[num],".6f")))
            f.write('Receive energy:\t\t {} joules\n'.format(format(receive_energy[num],".6f")))
            f.write('Total energy:\t\t {} joules\n\n'.format(format(total_energy[num],".6f")))

        f.write('#################  STATISTIC #################\n')

        f.write('### MEAN ###\n')
        f.write('--- Time ---\n')
        f.write('Mean process time:\t {} sec\n'.format(format(process_time_mean,".6f" )))
        f.write('Mean transmit time:\t {} sec\n'.format(format(transmit_time_mean,".6f" )))
        f.write('Mean receive time:\t {} sec\n'.format(format(receive_time_mean,".6f" )))
        f.write('Mean total time:\t {} sec\n'.format(format(total_time_mean,".6f" )))
        f.write('--- Energy ---\n')
        f.write('Mean process energy:\t {} joules\n'.format(format(process_energy_mean,".6f" )))
        f.write('Mean transmit energy:\t {} joules\n'.format(format(transmit_energy_mean,".6f" )))
        f.write('Mean receive energy:\t {} joules\n'.format(format(receive_energy_mean,".6f" )))
        f.write('Mean total energy:\t {} joules\n\n'.format(format(total_energy_mean,".6f" )))
        
        f.write('### VARIANCE ###\n')
        f.write('--- Time ---\n')
        f.write('Variance process time:\t {}\n'.format(format(process_time_variance,".12f" )))
        f.write('Variance transmit time:\t {}\n'.format(format(transmit_time_variance,".12f" )))
        f.write('Variance receive time:\t {}\n'.format(format(receive_time_variance,".12f" )))
        f.write('Variance total time:\t {}\n'.format(format(total_time_variance,".12f" )))
        f.write('--- Energy ---\n')
        f.write('Variance process energy:\t {}\n'.format(format(process_energy_variance,".12f" )))
        f.write('Variance transmit energy:\t {}\n'.format(format(transmit_energy_variance,".12f" )))
        f.write('Variance receive energy:\t {}\n'.format(format(receive_energy_variance,".12f" )))
        f.write('Variance total energy:\t {}\n'.format(format(total_energy_variance,".12f" )))
        

    print('- Txt done')

# ---------------------------------------------------------------------------------
def plots(process_energy, transmit_energy, receive_energy, total_energy, process_time, transmit_time, receive_time, total_time):

    # Create an array of x positions for each group of bars
    x = np.arange(len(total_energy)) +1

    # create the figure and subplots
    fig, axs = plt.subplots(2, 1, figsize=(6, 8))

    # plot the data on the first subplot
    width = 0.2
    axs[0].bar(x - width, process_energy, width, color='#264653', label='Process')
    axs[0].bar(x, transmit_energy, width, color='#2A9D8F', label='Transmit')
    axs[0].bar(x + width, receive_energy, width, color='#E9C46A', label='Receive')
    axs[0].bar(x + 2*width, total_energy, width,color='#F4A261', label='Total')

    axs[0].set_yscale('log')
    axs[0].set_ylabel('Joule - log10')
    axs[0].set_xlabel('Period no.')
    axs[0].set_title('Energy pr period')


    # plot the data on the second subplot
    axs[1].bar(x - width, process_time, width, color='#264653')
    axs[1].bar(x, transmit_time, width, color='#2A9D8F')
    axs[1].bar(x + width, receive_time, width, color='#E9C46A')
    axs[1].bar(x + 2*width, total_time, width,color='#F4A261')
    
    axs[1].set_yscale('log')
    axs[1].set_ylabel('Time(sec) - log10')
    axs[1].set_xlabel('Period no.')
    axs[1].set_title('Time pr period')

    # adjust spacing between subplots
    fig.subplots_adjust(hspace=0.3)

    # add a legend at the bottom of the figure
    fig.legend(loc='lower center', ncol=4)

    # adjust spacing between subplots and legend
    fig.subplots_adjust(hspace=0.3, bottom=0.1)

    # get name of the csv file
    global file_num
    plot_name = filenames[file_num]
    plot_name = plot_name.replace('.csv', '.png')

    print('- Plot done')

    # save the figure to a file
    fig.savefig(plot_name)

# ---------------------------------------------------------------------------------
def info(csv_files):
    for index, df in enumerate(csv_files):

        # read the csv file into a Dataframe
        data = pd.read_csv(df)

        data['start'] = (data['D0'] == 1) & (data['D0'].shift(1) == 0)
        
        data['series'] = data['start'].cumsum()
        
        num_periods = data[data['D0'] == 1]['series'].nunique()        
  
        print('File: {}\t Periods: {}'.format(filenames[index], num_periods))



# ---------------------------------------------------------------------------------
# ---------------------------------------------------------------------------------
if __name__ == "__main__":
    pin_energy = 326 
    file_num = 0
    # Path to dirtory for csv files
    dir_path = "/home/wrongside/test_folder/ENCRYPT"


    # Retreive a list of csv file
    #csv_files = [f for f in os.listdir(dir_path) if f.endswith('.csv')]
    #print('\nImported files: {}\n'.format(csv_files))
    csv_files = ['byte2.csv','byte4.csv','byte8.csv','byte16.csv']

    filenames = []
    for file in csv_files:
        filenames.append(str(file))

    # State the number of periods in each csv file
    info(csv_files)

    # Cut every file into segments of a period and pervious baseline
    segmented_files = segmentation(filenames)

    number_crunching(segmented_files)
