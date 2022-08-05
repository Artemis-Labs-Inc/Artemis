from time import sleep
from artemis_labs import artemis
import numpy as np 
import matplotlib.pyplot as plt 

def runVisualizer(state):    
    """
    Receive state of GUI, unpack inputs, plot normal distribution from inputs, 
    and update GUI with input 

    @param state: JSON object containing state of GUI
    """

    # Get inputs from GUI state 
    mean = float(state["input-mean"])
    variance = float(state["input-std"])
    samples = int(state["input-samples"])

    # Sample from normal distribution using inputs and create histogram
    a = np.hstack((np.random.normal(size=samples), np.random.normal(loc=mean, scale=variance, size=samples)))
    plt.hist(a, bins='auto')
    plt.xlabel("Value")
    plt.ylabel("Frequency")
    hist_plot_path = "normal.png"
    plt.savefig(hist_plot_path)
    plt.close()

    # Update image in GUI named 'pdf' with the figure we saved
    app.update('pdf', artemis.load_image('normal.png'))

    # Update caption above figure with plot name
    plot_name = "Histogram Of N(" + str(mean) + "," + str(variance) + ")"
    app.update('header', plot_name)

# Launch Artemis
app = artemis()

# Set event listener on button named 'update' which calls the function runVisualizer
# when the button is clicked
app.on("click", "update", runVisualizer)

# Idle forever
while True:
    sleep(1)

