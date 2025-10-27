!pip install numpy
!pip install cairosvg
!pip install pillow
!pip install numpy-stl
!pip install tqdm
!git clone https://github.com/positron48/svg2stl.git
%cd svg2stl
!pip install -r requirements.txt
!python svg2stl.py input.svg --thickness 1.0 --pixel_size 0.025
