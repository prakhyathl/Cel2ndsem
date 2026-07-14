import csv
import random

input_file = "C:\\Users\\Prakhyath L\\.gemini\\antigravity\\scratch\\loan_approval_dataset.csv"
output_file = "C:\\Users\\Prakhyath L\\.gemini\\antigravity\\scratch\\loan_approval_dataset_tmp.csv"

# Current columns (12): loan_id, employment_years, existing_emi, income_annum, loan_amount, loan_term, cibil_score, residential_assets_value, commercial_assets_value, luxury_assets_value, bank_asset_value, loan_status
# New columns (13): loan_id, age, employment_years, existing_emi, income_annum, loan_amount, loan_term, cibil_score, residential_assets_value, commercial_assets_value, luxury_assets_value, bank_asset_value, loan_status

with open(input_file, mode='r') as infile, open(output_file, mode='w', newline='') as outfile:
    reader = csv.reader(infile)
    writer = csv.writer(outfile)

    header = next(reader)
    new_header = [
        "loan_id", "age", "employment_years", "existing_emi",
        "income_annum", "loan_amount", "loan_term", "cibil_score",
        "residential_assets_value", "commercial_assets_value", "luxury_assets_value",
        "bank_asset_value", "loan_status"
    ]
    writer.writerow(new_header)

    for row in reader:
        if len(row) < 12:
            continue

        loan_id = row[0].strip()
        employment_years = int(row[1].strip())
        existing_emi = row[2].strip()
        income_annum = row[3].strip()
        loan_amount = row[4].strip()
        loan_term = row[5].strip()
        cibil_score = row[6].strip()
        res_assets = row[7].strip()
        comm_assets = row[8].strip()
        lux_assets = row[9].strip()
        bank_assets = row[10].strip()
        loan_status = row[11].strip()

        # Age is logically at least employment_years + 18, max 65
        min_age = employment_years + 18
        if min_age > 62:
            min_age = 62
        max_age = min(min_age + 20, 65)
        age = random.randint(min_age, max_age)

        new_row = [
            loan_id, str(age), str(employment_years), existing_emi,
            income_annum, loan_amount, loan_term, cibil_score,
            res_assets, comm_assets, lux_assets, bank_assets, loan_status
        ]
        writer.writerow(new_row)

import shutil
shutil.move(output_file, input_file)
print("Age column added successfully.")
