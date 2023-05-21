import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np


def plot(process, transmit, name):


    #rects1 = ax.bar([2,4,8,16], process, label='Process', color='#264653')
    #rects2 = ax.bar([2,4,8,16], transmit, label='Transmit', color='#2A9D8F')


    # Set the width of each bar
    bar_width = 0.35
    x = np.array([2, 4, 8, 16])


    fig, ax1 = plt.subplots()
    
    # Plot the first set of bars
    ax1.bar(x - bar_width/2, process, bar_width, label='Process',color='#264653')
    ax1.tick_params(axis='y', labelcolor='#264653')
    #ax1.set_ylim(0, 0.5)
    #ax1.set_yscale('log')
    ax1.set_yticks(np.arange(0, 1.1, 0.1))

    ax2 = ax1.twinx()
    # Plot the second set of bars next to the first one
    ax2.bar(x + bar_width/2, transmit, bar_width, label='Transmit',color='#2A9D8F')
    #ax2.set_ylabel('Y2', color='#2A9D8F')
    ax2.tick_params(axis='y', labelcolor='#2A9D8F')
    ax2.set_ylim(0, 45)
    ax2.set_yticks(np.arange(0, 45, 5))   


    # Add labels, title, and legend
    ax1.set_xticks(np.array([2,4,8,16]))
    #ax1.set_xticklabels(['2', '4', '8', '16'])

    ax1.set_xlabel('Payload size(bytes)')

    ax1.set_ylabel('Mean energy(joules)', color='#264653')
    ax2.set_ylabel('Mean energy(joules)', color='#2A9D8F')



    #ax.set_title('Confidence Intervals with encryption of payload')
    ax1.legend(loc='upper right',bbox_to_anchor=(0.45, 1))
    ax2.legend(loc='upper right', bbox_to_anchor=(0.75, 1))

    plotname = name + '.png'

    fig.savefig(plotname)


def plot2(process, transmit, name):


    #rects1 = ax.bar([2,4,8,16], process, label='Process', color='#264653')
    #rects2 = ax.bar([2,4,8,16], transmit, label='Transmit', color='#2A9D8F')


    # Set the width of each bar
    bar_width = 0.35
    x = np.array([2, 4, 8, 16])


    fig, ax1 = plt.subplots()
    
    # Plot the first set of bars
    ax1.bar(x - bar_width/2, process, bar_width, label='Process',color='#264653')
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


energy_process = [0.451388641695744, 0.4523165466285588, 0.45517496749641406, 0.4813206864000972]
energy_transmit = [26.336047971203346, 26.34101912314242, 29.48634139866269, 38.87711642714283]

time_process = [0.008112193548386938, 0.008119483870967475, 0.008201818181818338, 0.008605921052633289]
time_transmit = [0.04412322580645048, 0.044238903225807334, 0.049579610389610576, 0.06541414473684252]


plot(energy_process, energy_transmit, 'energy')

plot2(time_process, time_transmit, 'time')