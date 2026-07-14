import csv
import random
import shutil

input_file = "C:\\Users\\Prakhyath L\\.gemini\\antigravity\\scratch\\loan_approval_dataset.csv"
output_file = "C:\\Users\\Prakhyath L\\.gemini\\antigravity\\scratch\\loan_approval_dataset_new.csv"

# Original columns (13 columns): 
# loan_id, no_of_dependents, education, self_employed, income_annum, loan_amount, loan_term, cibil_score, residential_assets_value, commercial_assets_value, luxury_assets_value, bank_asset_value, loan_status

with open(input_file, mode='r') as infile, open(output_file, mode='w', newline='') as outfile:
    reader = csv.reader(infile)
    writer = csv.writer(outfile)
    
    header = next(reader)
    # We drop: no_of_dependents(1), education(2), self_employed(3)
    # We add: employment_years, existing_emi
    # New header: loan_id, employment_years, existing_emi, income_annum, loan_amount, loan_term, cibil_score, residential_assets_value, commercial_assets_value, luxury_assets_value, bank_asset_value, loan_status
    new_header = [
        "loan_id", "employment_years", "existing_emi", 
        "income_annum", "loan_amount", "loan_term", "cibil_score", 
        "residential_assets_value", "commercial_assets_value", "luxury_assets_value", 
        "bank_asset_value", "loan_status"
    ]
    writer.writerow(new_header)
    
    for row in reader:
        if len(row) < 13:
            continue
        
        loan_id = row[0].strip()
        income_annum = float(row[4].strip())
        loan_amount = row[5].strip()
        loan_term = row[6].strip()
        cibil_score = row[7].strip()
        res_assets = row[8].strip()
        comm_assets = row[9].strip()
        lux_assets = row[10].strip()
        bank_assets = row[11].strip()
        loan_status = row[12].strip()
        
        # Synthesize employment years (0 to 35)
        # Higher income / assets usually means more years of employment (older)
        base_years = int((income_annum / 10000000.0) * 15)
        employment_years = base_years + random.randint(0, 15)
        if employment_years > 40: employment_years = 40
        
        # Synthesize existing EMI (0 to 30% of monthly income)
        monthly_income = income_annum / 12.0
        # Give ~30% of people 0 existing EMI
        if random.random() < 0.3:
            existing_emi = 0
        else:
            existing_emi = int(monthly_income * random.uniform(0.05, 0.30))
            
        new_row = [
            loan_id, str(employment_years), str(existing_emi),
            str(int(income_annum)), loan_amount, loan_term, cibil_score,
            res_assets, comm_assets, lux_assets, bank_assets, loan_status
        ]
        writer.writerow(new_row)

shutil.move(output_file, input_file)
print("Dataset updated successfully.")
