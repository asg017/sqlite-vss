# https://github.com/facebookresearch/faiss/blob/74ee67aefc21859e02388f32f4876a0cbd21cabd/contrib/vecs_io.py#L20

import numpy as np

def ivecs_read(fname):
    a = np.fromfile(fname, dtype='int32')
    d = a[0]
    return a.reshape(-1, d + 1)[:, 1:].copy()


def fvecs_read(fname):
    return ivecs_read(fname).view('float32')

if __name__ == "__main__":
  fv = fvecs_read('tests/data/siftsmall/siftsmall_base.fvecs')
  print(fv.shape)
  print(fv[0])
  print(fv[1])
  for i, row in enumerate(fv[:10]):
    print(i, sum(row))