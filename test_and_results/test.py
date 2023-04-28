    df = pd.read_csv(filename2)
    print(df.to_string())

    #csvfile = open(filename, newline='')
    #reader = csv.DictReader(csvfile)
    
    
    # Total samples
    samples = df.options.display.max_rows
    print(samples)
    # Samples pr sec. on average
    #rows = list(reader)
    #print(reader)
    #samples_pr_sec = 

    #print(f"\n \t{filename}")
    #print("-----------------------------\n")
    #print("Samples: \t{}".format(samples))