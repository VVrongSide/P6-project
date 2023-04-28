
###########################
# Import libraies
import csv as cv
import sys
import pandas as pd 

###########################
# VARIABLES
filename3 = "result.csv"
filename2 = "test.csv"
mydataset = {
  'cars': ["BMW", "Volvo", "Ford"],
  'passings': [3, 7, 2]
}


# Timestamp(ms),Current(uA),D0,D1,D2,D3,D4,D5,D6,D7

###########################
# FUNCTIONS
def standard(filename):
    
    with open(filename2, mode='r') as csvfile:
        for x in csvfile:
            print(x)


def csv(filename):
    with open(filename2) as csvfile:
        reader = cv.reader(csvfile)
        total_salary = 0
        num_salary = 0
        for row in reader:
            if row[2] == 'Salary':
                pass
            else:
                total_salary = total_salary + float(row[2]) 
                num_salary += 1
        avg_salary = total_salary / num_salary
        print(avg_salary)

###########################
# MAIN
if __name__ == "__main__":

    
    standard(filename2)

    csv(filename2)