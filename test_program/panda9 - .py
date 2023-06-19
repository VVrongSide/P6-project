import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np


def plot(process_ct, process_pt, transmit_pt, transmit_ct, name):
    # Set the width of each bar
    bar_width = 0.40
    x = np.array([2, 4, 8, 16, 32, 64])

    fig, ax1 = plt.subplots()
    
    # Plot the first set of bars
    ax1.bar(x - bar_width*1.5, process_pt, bar_width, label='Process_PT',color='#264653')
    ax1.bar(x - bar_width*0.5, process_ct, bar_width, label='Process_CT',color='#2A9D8F')

    ax1.set_yticks(np.arange(0, 1, 0.1))

    ax2 = ax1.twinx()
    # Plot the second set of bars next to the first one
    ax2.bar(x + bar_width * 0.5, transmit_pt, bar_width, label='Transmit_PT',color='#E9C46A')
    ax2.bar(x + bar_width * 1.5, transmit_ct, bar_width, label='Transmit_CT',color='#F4A261')

    ax2.set_ylim(0, 85)
    ax2.set_yticks(np.arange(0, 85, 5))   


    # Add labels, title, and legend
    ax1.set_xticks(np.array([2,4,8,16,32,64]))
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
    # Set the width of each bar
    bar_width = 0.40
    x = np.array([2, 4, 8, 16, 32, 64])

    fig, ax1 = plt.subplots()
    
    # Plot the first set of bars
    ax1.bar(x - bar_width*1.5, process_pt, bar_width, label='Process_PT',color='#264653')
    ax1.bar(x - bar_width*0.5, process_ct, bar_width, label='Process_CT',color='#2A9D8F')

    ax1.set_yticks(np.arange(0, 0.025, 0.005))

    ax2 = ax1.twinx()
    # Plot the second set of bars next to the first one
    ax2.bar(x + bar_width * 0.5, transmit_pt, bar_width, label='Transmit_PT',color='#E9C46A')
    ax2.bar(x + bar_width * 1.5, transmit_ct, bar_width, label='Transmit_CT',color='#F4A261')

    ax2.set_ylim(0, 0.08)

    # Add labels, title, and legend
    ax1.set_xticks(np.array([2,4,8,16, 32, 64]))
    #ax1.set_xticklabels(['2', '4', '8', '16'])

    ax1.set_xlabel('Payload size(bytes)')

    ax1.set_ylabel('Mean time(seconds) - Process', color='#000000')
    ax2.set_ylabel('Mean time(seconds) - Transmit', color='#000000')


    #ax.set_title('Confidence Intervals with encryption of payload')
    ax1.legend(loc='upper right',bbox_to_anchor=(0.45, 1))
    ax2.legend(loc='upper right', bbox_to_anchor=(0.75, 1))

    plotname = name + '.png'

    fig.savefig(plotname)


 
  


energy_process_PT = [0.4123577860135196, 0.41347071381903494, 0.41401889127749275, 0.41465984144878504]
energy_transmit_PT = [26.328298053372432, 26.347478701293326, 29.50852424039708, 38.90526402768336]
time_process_PT = [0.007419935483871026, 0.007425161290322835, 0.007436688311688801, 0.007452026143791295]
time_transmit_PT = [0.044123161290322024, 0.04423987096774185, 0.04957902597402543, 0.06539790849673183]

# energy
Process mean: [0.4121202474644624, 0.6130992657298497]
Transmit mean: [51.33030783998544, 79.23736302153728]

# timea
Process mean: [0.007464333333334881, 0.011204374999998996]
Transmit mean: [0.08679793333333248, 0.1347213888888884]

#energy_process_CT = [0.451388641695744, 0.4523165466285588, 0.45517496749641406, 0.4813206864000972]
#energy_transmit_CT = [26.336047971203346, 26.34101912314242, 29.48634139866269, 38.87711642714283]
#time_process_CT = [0.008112193548386938, 0.008119483870967475, 0.008201818181818338, 0.008605921052633289]
#time_transmit_CT = [0.04412322580645048, 0.044238903225807334, 0.049579610389610576, 0.06541414473684252]


energy_process_CT = [0.451388641695744, 0.4523165466285588, 0.45517496749641406, 0.4813206864000972, 0.6113221129977252, 1.003126055634893]
energy_transmit_CT = [26.336047971203346, 26.34101912314242, 29.48634139866269, 38.87711642714283, 51.32563651707467, 79.33442188208058]
time_process_CT = [0.008112193548386938, 0.008119483870967475, 0.008201818181818338, 0.008605921052633289, 0.010874026845636922, 0.018018811188811698]
time_transmit_CT = [0.04412322580645048, 0.044238903225807334, 0.049579610389610576, 0.06541414473684252, 0.08682241610738264, 0.13476244755244768]


#plot(process, transmit, name)
#plot(energy_process_CT, energy_process_PT, energy_transmit_PT, energy_transmit_CT, 'energy')
plot2(time_process_CT, time_process_PT, time_transmit_PT, time_transmit_CT, 'time')

#plot2(time_process, time_transmit, 'time')