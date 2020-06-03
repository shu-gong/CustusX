import sys
import os
import shutil
import SimpleITK as sitk
import skimage.io as io
import numpy as np

class custusVolume():
    def __init__(self,volume_path):
        self.volume_path = volume_path
        self.base_path = os.path.dirname(volume_path)

        file_name = os.path.basename(volume_path)
        root_ext = os.path.splitext(file_name)
        self.volume_name = root_ext[0]
        self.volume_type = root_ext[1]

    def duplicate(self,new_path):
        new_path = os.path.splitext(new_path)[0]  # Drop extension
        mhd_path = os.path.join(self.base_path,self.volume_name + '.mhd')
        new_mhd_path = new_path + '.mhd'
        shutil.copy(mhd_path, new_mhd_path)

        raw_path = os.path.join(self.base_path, self.volume_name + '.raw')
        new_raw_path = new_path + '.raw'
        shutil.copy(raw_path, new_raw_path)

    def load_volume(self):
        mhd_path = os.path.join(self.base_path, self.volume_name + '.mhd')
        self.volume = sitk.ReadImage(mhd_path)

        # print('Image shape: ', self.volume.shape)
        print('Image origin: ', self.volume.GetOrigin())
        print('Image spacing: ', self.volume.GetSpacing())

    def get_array(self,dtype='float32'):
        image_array = sitk.GetArrayFromImage(self.volume)
        return image_array.astype(dtype)

    def save_volume(self, data, path):
        # Create new volume
        new_volume = sitk.GetImageFromArray(data.astype)
        new_volume.CopyInformation(self.volume)

        # Create path
        base_path = os.path.dirname(path)
        file_name = os.path.basename(path)
        root_ext = os.path.splitext(file_name)
        volume_name = root_ext[0]
        mhd_path = os.path.join(base_path, volume_name + '.mhd')

        # Write to file
        sitk.WriteImage(new_volume, mhd_path)







