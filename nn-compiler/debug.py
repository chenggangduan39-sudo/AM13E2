import os

def mode():
    if 'qnn_debug' in os.environ:
        return os.environ['qnn_debug']
    return None

def size2str(nbytes):
    if nbytes >= 1024 * 1024 * 1024:
        return f'{nbytes/(1024*1024*1024):.2f}GB'
    if nbytes >= 1024 * 1024:
        return f'{nbytes/(1024*1024):.2f}MB'
    if nbytes >= 1024:
        return f'{nbytes/1024:.2f}KB'
    return f'{nbytes}B'
