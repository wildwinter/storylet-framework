import random
import copy

def shuffle_array(array):
    """Shuffle an array in place."""
    for i in range(len(array) - 1, 0, -1):
        j = random.randint(0, i)
        array[i], array[j] = array[j], array[i]
    return array

def copy_object(original):
    """Create a deep copy of the original object."""
    return copy.deepcopy(original)

def update_object(original, additions):
    """Update the original dictionary with values from the additions dictionary."""
    for var_name, value in additions.items():
        original[var_name] = value