import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np


def plot(process_ct, process_pt, transmit_pt, transmit_ct, name):


    #rects1 = ax.bar([2,4,8,16], process, label='Process', color='#264653')
    #rects2 = ax.bar([2,4,8,16], transmit, label='Transmit', color='#2A9D8F')


    # Set the width of each bar
    bar_width = 0.40
    x = np.array([2, 4, 8, 16])


    fig, ax1 = plt.subplots()
    
    # Plot the first set of bars
    ax1.bar(x - bar_width*1.5, process_pt, bar_width, label='Process_pt',color='#264653')
    #ax1.tick_params(axis='y', labelcolor='#264653')
    ax1.bar(x - bar_width*0.5, process_ct, bar_width, label='Process_ct',color='#2A9D8F')
    #ax1.tick_params(axis='y', labelcolor='#2A9D8F')
    
    #ax1.set_ylim(0, 0.5)
    #ax1.set_yscale('log')
    ax1.set_yticks(np.arange(0, 1, 0.1))

    ax2 = ax1.twinx()
    # Plot the second set of bars next to the first one
    ax2.bar(x + bar_width * 0.5, transmit_pt, bar_width, label='Transmit_pt',color='#E9C46A')
    #ax2.tick_params(axis='y', labelcolor='#E9C46A')
    ax2.bar(x + bar_width * 1.5, transmit_ct, bar_width, label='Transmit_ct',color='#F4A261')
    #ax2.tick_params(axis='y', labelcolor='#F4A261')

    ax2.set_ylim(0, 40)
    ax2.set_yticks(np.arange(0, 40, 5))   


    # Add labels, title, and legend
    ax1.set_xticks(np.array([2,4,8,16]))
    #ax1.set_xticklabels(['2', '4', '8', '16'])

    ax1.set_xlabel('Payload size(bytes)')

    ax1.set_ylabel('Mean energy(joules) - Process', color='#000000')
    ax2.set_ylabel('Mean energy(joules) - Transmit', color='#000000')



    #ax.set_title('Confidence Intervals with encryption of payload')
    ax1.legend(loc='upper right',bbox_to_anchor=(0.45, 1))
    ax2.legend(loc='upper right', bbox_to_anchor=(0.75, 1))

    plotname = name + '.png'

    fig.savefig(plotname)


def plot2(process_ct, process_pt, transmit_pt, transmit_ct, name):


    #rects1 = ax.bar([2,4,8,16], process, label='Process', color='#264653')
    #rects2 = ax.bar([2,4,8,16], transmit, label='Transmit', color='#2A9D8F')


    # Set the width of each bar
    bar_width = 0.40
    x = np.array([2, 4, 8, 16])


    fig, ax1 = plt.subplots()
    
    # Plot the first set of bars
    ax1.bar(x - bar_width/4, process_pt, bar_width, label='Process',color='#264653')
    ax1.tick_params(axis='y', labelcolor='#264653')
    #ax1.set_ylim(0, 0.5)
    #ax1.set_yscale('log')
    ax1.set_yticks(np.arange(0, 0.025, 0.005))

    ax2 = ax1.twinx()
    # Plot the second set of bars next to the first one
    ax2.bar(x + bar_width/2, transmit, bar_width, label='Transmit',color='#2A9D8F')
    #ax2.set_ylabel('Y2', color='#2A9D8F')
    ax2.tick_params(axis='y', labelcolor='#2A9D8F')
    ax2.set_ylim(0, 0.08)
    #ax2.set_yticks(np.arange(0, 0.1, 5))   
    #ax2.set_yscale('log')

    # Add labels, title, and legend
    ax1.set_xticks(np.array([2,4,8,16]))
    #ax1.set_xticklabels(['2', '4', '8', '16'])

    ax1.set_xlabel('Payload size(bytes)')

    ax1.set_ylabel('Mean time(seconds)', color='#264653')
    ax2.set_ylabel('Mean time(seconds)', color='#2A9D8F')



    #ax.set_title('Confidence Intervals with encryption of payload')
    ax1.legend(loc='upper right',bbox_to_anchor=(0.45, 1))
    ax2.legend(loc='upper right', bbox_to_anchor=(0.75, 1))

    plotname = name + '.png'

    fig.savefig(plotname)



energy_process_PT = [0.4152900985676647, 0.41389834828303046, 0.41233425193269696, 0.41260862908470886]
energy_transmit_PT = [26.34334369553797, 26.339812140193892, 29.485312914028952, 38.86816327368588]
time_process_PT = [0.007419806451613863, 0.007422064516128425, 0.00742655844155787, 0.007434640522875149]
time_transmit_PT = [0.04412316129032262, 0.04423929032258096, 0.049579545454545605, 0.06539751633986879]

energy_process_CT = [0.45297263342967486, 0.4514453051230901, 0.45517496749641406, 0.48047704259959095]
energy_transmit_CT = [26.341678890561834, 26.334281497765698, 29.48634139866269, 38.85072876186124]
time_process_CT = [0.008115225806451486, 0.008116903225807002, 0.008201818181818338, 0.00858828947368403]
time_transmit_CT = [0.044123548387097464, 0.04423890322580602, 0.049579610389610576, 0.0654140131578945]


#plot(process, transmit, name)
plot(energy_process_CT, energy_process_PT, energy_transmit_PT, energy_transmit_CT, 'energy')

#plot2(time_process, time_transmit, 'time')