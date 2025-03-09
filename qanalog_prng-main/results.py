import os
import re
from tabulate import tabulate

result_dir = 'test_results'
filename_pattern = re.compile(r'e(\d+)_w(\d+)_seed(\d+)')

def parse_test_file(file_path):
    with open(file_path, 'r') as f:
        content = f.read()

    if "All tests were passed" in content:
        return "Pass all", []
    
    failed_tests = []
    in_failed_section = False
    first_separator = False  # Flag para detectar a primeira linha de traços
    
    for line in content.split('\n'):
        line = line.strip()
        
        # Detecta início da seção de falhas
        if "The following tests gave p-values" in line:
            in_failed_section = True
            first_separator = False
            continue
        
        # Detecta linhas de separação
        if line.startswith('-') and len(line) >= 20:  # Assume que a linha tem pelo menos 20 '-'
            if in_failed_section:
                first_separator = True  # Marca que encontrou a primeira linha de traços
            continue
        
        # Captura testes após a primeira linha de traços
        if in_failed_section and first_separator and line:
            # Ignora a própria linha de traços e linhas vazias
            if not line.startswith('-') and not line.isspace():
                parts = line.split(maxsplit=2)  # Separa número, nome e p-value
                if len(parts) >= 2:
                    test_number = parts[0]
                    test_name = parts[1].strip()
                    failed_tests.append(f"{test_number}: {test_name}")
    
    return "Fail", list(set(failed_tests))  # Remove duplicatas

def main():
    results = []
    
    for filename in os.listdir(result_dir):
        if not filename.endswith('.txt'):
            continue
            
        match = filename_pattern.search(filename)
        if not match:
            continue
            
        e, w, seed = match.groups()
        test_name = filename.split('_')[-1].replace('.txt', '')
        file_path = os.path.join(result_dir, filename)
        
        status, failed = parse_test_file(file_path)
        
        results.append({
            'e': e,
            'w': w,
            'seed': seed,
            'Teste': test_name,
            'Resultado': f"{status} ({', '.join(failed)})" if failed else status
        })
    
    results.sort(key=lambda x: (int(x['e']), int(x['w']), int(x['seed'])))
    
    table = [[
        res['e'],
        res['w'],
        res['seed'],
        res['Teste'],
        res['Resultado']
    ] for res in results]
    
    print(tabulate(table, headers=['e', 'w', 'seed', 'Teste', 'Resultado'], tablefmt='grid'))

if __name__ == '__main__':
    main()
