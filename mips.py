import time
from mipsim import MIPSSimulator
 
def execute_program(binary_path):
    sim = MIPSSimulator()
    sim.load_program(binary_path)
 
    sim.reset()
 
    start_time = time.time()
 
    while sim.running:
        sim.tick()
 
    end_time = time.time()
 
    instructions = sim.get_instruction_count()
    elapsed_time = end_time - start_time
    mips = instructions / (elapsed_time * 1000000)
    return mips
 
binary_path = "./tool/engine-eval -c res-kws/engine.cfg -wav sheng-wen/recognition.pcm"
mips = execute_program(binary_path)
print(f"MIPS: {mips}")
