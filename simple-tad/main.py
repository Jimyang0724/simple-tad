from typing import Tuple
import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
import os
import simple_tad


def read_hi_c_data(filename: str, bin_size: int, bin1_min: int, bin1_max: int, bin2_min: int, bin2_max: int) -> Tuple[np.ndarray, int]:
    with open(filename, 'r') as file:
        # skip header
        file.readline()

        # get edge size
        edge_size = (max(bin1_max, bin2_max) -
                     min(bin1_min, bin2_min)) // bin_size + 1

        # create data array
        data = np.zeros((edge_size, edge_size), dtype=np.float32)

        # read data
        for line in file:
            chr, bin1, bin2, rescaled_intensity = line.split(
                ',')
            bin1 = int(bin1)
            bin2 = int(bin2)
            rescaled_intensity = float(rescaled_intensity)
            if bin1_min <= bin1 <= bin1_max and bin2_min <= bin2 <= bin2_max:
                row = (bin1 - min(bin1_min, bin2_min)) // bin_size
                col = (bin2 - min(bin1_min, bin2_min)) // bin_size
                data[row, col] = rescaled_intensity
                data[col, row] = rescaled_intensity
            else:
                print(
                    f'chr: {chr} bin1: {bin1} bin2: {bin2} rescaled_intensity: {rescaled_intensity}')

    return data, edge_size


def main():
    # read data
    global_data, edge_size = read_hi_c_data(
        "./data/GM12878_MboI_chr6.csv", 5000, 140000, 170590000, 160000, 170610000)

    # if output folder does not exist, create it
    if not os.path.exists('./output'):
        os.makedirs('./output')

    LOCAL_SIZE = 500

    for i in range(0, edge_size, LOCAL_SIZE):
        local_data = global_data[i:i+LOCAL_SIZE, i:i+LOCAL_SIZE]

        assert local_data.shape[0] == local_data.shape[1]

        coords = simple_tad.calculate_tad_coords(
            local_data.reshape(local_data.shape[0]**2),
            local_data.shape[0],
            5000
        )

        print("coords done")

        # ensure all bins can be seen
        plt.figure(dpi=1000)

        # plot heatmap and save to file
        plt.imshow(
            local_data,
            cmap=mpl.colors.LinearSegmentedColormap.from_list(
                "",
                [(1, 1, 1), (0.75, 0.0, 0.0)]
            ),
            interpolation='none'
        )

        # add TADs
        for coord in coords:
            plt.gca().add_patch(
                mpl.patches.Rectangle(
                    (coord[0], coord[0]),
                    coord[1] - coord[0],
                    coord[1] - coord[0],
                    edgecolor='violet',
                    facecolor='none',
                    linewidth=0.5
                )
            )

        # add colorbar
        plt.colorbar()

        plt.savefig(f'./output/heatmap-{i}.png')

        plt.close()


if __name__ == '__main__':
    main()
