#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <windows.h>

#define MAX_RECORDS 4500
#define K_NEIGHBORS 9

typedef struct {
    int age;
    int employmentYears;
    double existingEmi;
    double incomeAnnum;
    double loanAmount;
    int loanTermYears;
    int cibilScore;
    double totalAssets;
    int loanStatus;       // 1 = Approved, 0 = Rejected
} KaggleApplicant;

typedef struct {
    double distance;
    int index;
    int status;
} Neighbor;

KaggleApplicant dataset[MAX_RECORDS];
int datasetSize = 0;

double minAge, maxAge;
double minEmployment, maxEmployment;
double minEmi, maxEmi;
double minIncome, maxIncome;
double minLoanAmt, maxLoanAmt;
double minTerm, maxTerm;
double minCibil, maxCibil;
double minTotalAssets, maxTotalAssets;

// GTK Entry & Combo widgets
GtkWidget *entryName;
GtkWidget *entryAge;
GtkWidget *entryEmployment;
GtkWidget *entryExistingEmi;
GtkWidget *entryIncome;
GtkWidget *entryLoan;
GtkWidget *entryTenure;
GtkWidget *entryCredit;
GtkWidget *entryTotalAssets;

// Results layout widgets
GtkWidget *labelDecision;
GtkWidget *labelScore;
GtkWidget *labelEmi;
GtkWidget *labelDti;
GtkWidget *labelReasons;
GtkWidget *labelRiskLevel;
GtkWidget *decisionBox;
GtkWidget *scoreProgress;
GtkWidget *riskMeter;

double currentDti = 0.0;

const char *APP_CSS =
    "window { background: #f3f6fb; font-family: Sans; font-size: 14px; }"
    ".header { background: #0b2f5b; color: white; padding: 20px; }"
    ".title { color: white; font-size: 26px; font-weight: bold; }"
    ".subtitle { color: #c9d8ea; font-size: 12px; }"
    ".panel { background: #ffffff; border: 1px solid #d8e0ea; border-radius: 8px; padding: 16px; }"
    ".section-title { color: #0b2f5b; font-size: 17px; font-weight: bold; margin-bottom: 8px; }"
    ".field-label { color: #334155; font-weight: bold; }"
    ".result-box { border-radius: 8px; padding: 16px; }"
    ".decision-default { background: #e8eef6; color: #0f172a; }"
    ".decision-approved { background: #dcfce7; color: #166534; }"
    ".decision-review { background: #ffedd5; color: #9a3412; }"
    ".decision-rejected { background: #fee2e2; color: #991b1b; }"
    ".decision-text { font-size: 26px; font-weight: bold; }"
    ".metric-label { color: #475569; font-weight: bold; }"
    ".factors { background: #f8fafc; color: #1e293b; border: 1px solid #e2e8f0; border-radius: 6px; padding: 12px; font-family: monospace; font-size: 12px; }"
    ".primary-button { background: #0b5cab; color: white; border-radius: 6px; padding: 8px 14px; font-weight: bold; }"
    ".secondary-button { background: #e2e8f0; color: #0f172a; border-radius: 6px; padding: 8px 14px; }"
    ".demo-button { background: #0f766e; color: white; border-radius: 6px; padding: 8px 14px; }"
    ".score-low progress { background: #dc2626; }"
    ".score-medium progress { background: #f59e0b; }"
    ".score-high progress { background: #16a34a; }"
    "progressbar trough { min-height: 14px; border-radius: 8px; background: #e5e7eb; }"
    "progressbar progress { min-height: 14px; border-radius: 8px; }";

void addStyle(GtkWidget *widget, const char *className) {
    gtk_style_context_add_class(
        gtk_widget_get_style_context(widget),
        className);
}

void removeStyle(GtkWidget *widget, const char *className) {
    gtk_style_context_remove_class(
        gtk_widget_get_style_context(widget),
        className);
}

void setDecisionStyle(const char *decision) {
    removeStyle(decisionBox, "decision-approved");
    removeStyle(decisionBox, "decision-review");
    removeStyle(decisionBox, "decision-rejected");
    removeStyle(decisionBox, "decision-default");

    if (strcmp(decision, "APPROVED") == 0)
        addStyle(decisionBox, "decision-approved");
    else if (strcmp(decision, "REVIEW") == 0)
        addStyle(decisionBox, "decision-review");
    else if (strcmp(decision, "REJECTED") == 0)
        addStyle(decisionBox, "decision-rejected");
    else
        addStyle(decisionBox, "decision-default");
}

void setProgressStyle(int score) {
    removeStyle(scoreProgress, "score-low");
    removeStyle(scoreProgress, "score-medium");
    removeStyle(scoreProgress, "score-high");

    if (score >= 75)
        addStyle(scoreProgress, "score-high");
    else if (score >= 50)
        addStyle(scoreProgress, "score-medium");
    else
        addStyle(scoreProgress, "score-low");
}

void loadCss(void) {
    GtkCssProvider *provider = gtk_css_provider_new();

    gtk_css_provider_load_from_data(
        provider,
        APP_CSS,
        -1,
        NULL);

    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    g_object_unref(provider);
}

double calculateEmi(double principal, int years) {
    double annualRate = 10.0;
    double monthlyRate = annualRate / (12 * 100.0);
    int months = years * 12;
    double power = pow(1 + monthlyRate, months);

    return principal * monthlyRate * power / (power - 1);
}

void showErrorDialog(const char *message) {
    GtkWidget *dialog =
        gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "%s",
            message);

    gtk_window_set_title(GTK_WINDOW(dialog), "Error");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Custom CSV parsing helpers
void trim(char *str) {
    int l = strlen(str);
    while (l > 0 && (str[l-1] == ' ' || str[l-1] == '\r' || str[l-1] == '\n' || str[l-1] == '\t')) {
        str[--l] = '\0';
    }
    int start = 0;
    while (str[start] == ' ' || str[start] == '\t') {
        start++;
    }
    if (start > 0) {
        memmove(str, str + start, l - start + 1);
    }
}

char* get_token(char **line) {
    if (*line == NULL || **line == '\0') return NULL;
    char *start = *line;
    char *p = *line;
    while (*p != '\0' && *p != ',') {
        p++;
    }
    if (*p == ',') {
        *p = '\0';
        *line = p + 1;
    } else {
        *line = NULL;
    }
    return start;
}

int loadDataset(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        return 0;
    }
    char line[1024];
    // Skip header
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return 0;
    }
    
    datasetSize = 0;
    while (fgets(line, sizeof(line), fp) && datasetSize < MAX_RECORDS) {
        KaggleApplicant *a = &dataset[datasetSize];
        char *linePtr = line;
        char *tokens[13];
        
        for (int i = 0; i < 13; i++) {
            tokens[i] = get_token(&linePtr);
            if (!tokens[i]) break;
            trim(tokens[i]);
        }
        
        if (!tokens[12]) continue; // incomplete row
        
        a->age = atoi(tokens[1]);
        a->employmentYears = atoi(tokens[2]);
        a->existingEmi = atof(tokens[3]);
        a->incomeAnnum = atof(tokens[4]);
        a->loanAmount = atof(tokens[5]);
        a->loanTermYears = atoi(tokens[6]);
        a->cibilScore = atoi(tokens[7]);
        double res = atof(tokens[8]);
        double comm = atof(tokens[9]);
        double lux = atof(tokens[10]);
        double bank = atof(tokens[11]);
        a->totalAssets = res + comm + lux + bank;
        a->loanStatus = (strcasecmp(tokens[12], "Approved") == 0) ? 1 : 0;
        
        // Track bounds for normalization
        if (datasetSize == 0) {
            minAge = maxAge = a->age;
            minEmployment = maxEmployment = a->employmentYears;
            minEmi = maxEmi = a->existingEmi;
            minIncome = maxIncome = a->incomeAnnum;
            minLoanAmt = maxLoanAmt = a->loanAmount;
            minTerm = maxTerm = a->loanTermYears;
            minCibil = maxCibil = a->cibilScore;
            minTotalAssets = maxTotalAssets = a->totalAssets;
        } else {
            if (a->age < minAge) minAge = a->age;
            if (a->age > maxAge) maxAge = a->age;
            
            if (a->employmentYears < minEmployment) minEmployment = a->employmentYears;
            if (a->employmentYears > maxEmployment) maxEmployment = a->employmentYears;
            
            if (a->existingEmi < minEmi) minEmi = a->existingEmi;
            if (a->existingEmi > maxEmi) maxEmi = a->existingEmi;
            
            if (a->incomeAnnum < minIncome) minIncome = a->incomeAnnum;
            if (a->incomeAnnum > maxIncome) maxIncome = a->incomeAnnum;
            
            if (a->loanAmount < minLoanAmt) minLoanAmt = a->loanAmount;
            if (a->loanAmount > maxLoanAmt) maxLoanAmt = a->loanAmount;
            
            if (a->loanTermYears < minTerm) minTerm = a->loanTermYears;
            if (a->loanTermYears > maxTerm) maxTerm = a->loanTermYears;
            
            if (a->cibilScore < minCibil) minCibil = a->cibilScore;
            if (a->cibilScore > maxCibil) maxCibil = a->cibilScore;
            
            if (a->totalAssets < minTotalAssets) minTotalAssets = a->totalAssets;
            if (a->totalAssets > maxTotalAssets) maxTotalAssets = a->totalAssets;
        }
        
        datasetSize++;
    }
    fclose(fp);
    return datasetSize;
}

// Compare distance for KNN sorting
int compareNeighbors(const void *a, const void *b) {
    double distA = ((Neighbor*)a)->distance;
    double distB = ((Neighbor*)b)->distance;
    if (distA < distB) return -1;
    if (distA > distB) return 1;
    return 0;
}

// Min-Max scaling helper macro
#define NORM(val, min, max) (((max) - (min)) > 0 ? ((val) - (min)) / ((max) - (min)) : 0.0)

double computeDistance(KaggleApplicant *a, KaggleApplicant *b) {
    double dist = 0.0;
    
    double d_age  = NORM(a->age, minAge, maxAge) - NORM(b->age, minAge, maxAge);
    double d_emp  = NORM(a->employmentYears, minEmployment, maxEmployment) - NORM(b->employmentYears, minEmployment, maxEmployment);
    double d_emi  = NORM(a->existingEmi, minEmi, maxEmi) - NORM(b->existingEmi, minEmi, maxEmi);
    double d_inc  = NORM(a->incomeAnnum, minIncome, maxIncome) - NORM(b->incomeAnnum, minIncome, maxIncome);
    double d_loan = NORM(a->loanAmount, minLoanAmt, maxLoanAmt) - NORM(b->loanAmount, minLoanAmt, maxLoanAmt);
    double d_term = NORM(a->loanTermYears, minTerm, maxTerm) - NORM(b->loanTermYears, minTerm, maxTerm);
    double d_cibil= NORM(a->cibilScore, minCibil, maxCibil) - NORM(b->cibilScore, minCibil, maxCibil);
    double d_assets = NORM(a->totalAssets, minTotalAssets, maxTotalAssets) - NORM(b->totalAssets, minTotalAssets, maxTotalAssets);
    
    dist += d_age   * d_age   * 0.8;  // Age: retirement-risk factor
    dist += d_emp   * d_emp   * 0.8;  // Employment Years: stability indicator
    dist += d_emi   * d_emi   * 1.5;  // Existing EMI: current debt burden (high impact)
    dist += d_inc   * d_inc   * 1.0;  // Annual Income: repayment capacity
    dist += d_loan  * d_loan  * 1.2;  // Loan Amount: risk magnitude
    dist += d_term  * d_term  * 0.5;  // Loan Term: affects EMI, already in DTI
    dist += d_cibil * d_cibil * 2.5;  // CIBIL Score: creditworthiness (highest impact)
    dist += d_assets* d_assets* 1.2;  // Total Assets: collateral strength
    
    return sqrt(dist);
}

int evaluateKNN(KaggleApplicant *input, int k, double *confidence, char *reasonsOut) {
    if (datasetSize == 0) return 0;
    
    Neighbor *neighbors = malloc(sizeof(Neighbor) * datasetSize);
    for (int i = 0; i < datasetSize; i++) {
        neighbors[i].distance = computeDistance(input, &dataset[i]);
        neighbors[i].index = i;
        neighbors[i].status = dataset[i].loanStatus;
    }
    
    qsort(neighbors, datasetSize, sizeof(Neighbor), compareNeighbors);
    
    int approvedVotes = 0;
    int rejectedVotes = 0;
    
    for (int i = 0; i < k; i++) {
        if (neighbors[i].status == 1) {
            approvedVotes++;
        } else {
            rejectedVotes++;
        }
    }
    
    int decision = (approvedVotes >= rejectedVotes) ? 1 : 0;
    int majorityVotes = (decision == 1) ? approvedVotes : rejectedVotes;
    *confidence = ((double)majorityVotes / k) * 100.0;
    
    char temp[512];
    strcpy(reasonsOut, "Closest matching records in Kaggle Dataset:\n\n");
    for (int i = 0; i < 4; i++) {
        int idx = neighbors[i].index;
        snprintf(temp, sizeof(temp), 
                 "#%d -> CIBIL: %d | Income: %.1f L | Loan: %.1f L | Term: %d yrs | Status: %s\n",
                 i + 1,
                 dataset[idx].cibilScore,
                 dataset[idx].incomeAnnum / 100000.0,
                 dataset[idx].loanAmount / 100000.0,
                 dataset[idx].loanTermYears,
                 dataset[idx].loanStatus ? "APPROVED" : "REJECTED");
        strcat(reasonsOut, temp);
    }
    
    free(neighbors);
    return decision;
}

// Build a path relative to the exe's own directory
static void buildExePath(char *outBuf, int bufLen, const char *filename) {
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    // Strip the exe filename to get the folder
    char *lastSlash = strrchr(exePath, '\\');
    if (lastSlash) *(lastSlash + 1) = '\0';
    snprintf(outBuf, bufLen, "%s%s", exePath, filename);
}

void saveLoanRecord(const char *name, KaggleApplicant *a, int decision, double confidence) {
    char filePath[MAX_PATH];
    buildExePath(filePath, sizeof(filePath), "loan_records.txt");
    FILE *file = fopen(filePath, "a");
    if (file == NULL) return;

    fprintf(file,
            "Applicant Name: %s\n"
            "Age: %d\n"
            "Employment Years: %d\n"
            "Existing EMI: %.2f\n"
            "Annual Income: %.2f\n"
            "Loan Amount: %.2f\n"
            "Loan Term (Years): %d\n"
            "CIBIL Score: %d\n"
            "Prediction Status: %s\n"
            "Confidence Score: %.1f%%\n"
            "------------------------------\n",
            name,
            a->age,
            a->employmentYears,
            a->existingEmi,
            a->incomeAnnum,
            a->loanAmount,
            a->loanTermYears,
            a->cibilScore,
            decision == 1 ? "APPROVED" : (decision == 2 ? "REVIEW" : "REJECTED"),
            confidence);

    fclose(file);
}

void writeSystemLog(const char *name, int decision, double confidence) {
    char filePath[MAX_PATH];
    buildExePath(filePath, sizeof(filePath), "system_log.txt");
    FILE *file = fopen(filePath, "a");
    if (file == NULL) return;

    time_t now = time(NULL);
    struct tm *localTime = localtime(&now);
    char timestamp[30];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localTime);

    fprintf(file,
            "[%s] %s | %s | KNN Confidence: %.1f%%\n",
            timestamp,
            name,
            decision == 1 ? "APPROVED" : (decision == 2 ? "REVIEW" : "REJECTED"),
            confidence);

    fclose(file);
}

void readApplicantFromForm(KaggleApplicant *a, char *nameBuffer, int nameLen) {
    const char *nameText = gtk_entry_get_text(GTK_ENTRY(entryName));
    strncpy(nameBuffer, nameText, nameLen - 1);
    nameBuffer[nameLen - 1] = '\0';

    a->age = atoi(gtk_entry_get_text(GTK_ENTRY(entryAge)));
    a->employmentYears = atoi(gtk_entry_get_text(GTK_ENTRY(entryEmployment)));
    a->existingEmi = atof(gtk_entry_get_text(GTK_ENTRY(entryExistingEmi)));

    a->incomeAnnum = atof(gtk_entry_get_text(GTK_ENTRY(entryIncome)));
    a->loanAmount = atof(gtk_entry_get_text(GTK_ENTRY(entryLoan)));
    a->loanTermYears = atoi(gtk_entry_get_text(GTK_ENTRY(entryTenure)));
    a->cibilScore = atoi(gtk_entry_get_text(GTK_ENTRY(entryCredit)));
    a->totalAssets = atof(gtk_entry_get_text(GTK_ENTRY(entryTotalAssets)));
}

int validateApplicant(KaggleApplicant *a, const char *name) {
    if (strlen(name) == 0) {
        showErrorDialog("Applicant Name cannot be empty.");
        return 0;
    }
    if (a->age < 18 || a->age > 100) {
        showErrorDialog("Applicant Age must be between 18 and 100.");
        return 0;
    }
    if (a->employmentYears < 0 || a->employmentYears > 60) {
        showErrorDialog("Employment Years must be between 0 and 60.");
        return 0;
    }
    if (a->existingEmi < 0) {
        showErrorDialog("Existing EMI cannot be negative.");
        return 0;
    }
    if (a->incomeAnnum <= 0) {
        showErrorDialog("Annual Income must be greater than 0.");
        return 0;
    }
    if (a->loanAmount <= 0) {
        showErrorDialog("Loan Amount must be greater than 0.");
        return 0;
    }
    if (a->loanTermYears <= 0 || a->loanTermYears > 40) {
        showErrorDialog("Loan Tenure must be between 1 and 40 years.");
        return 0;
    }
    if (a->cibilScore < 300 || a->cibilScore > 900) {
        showErrorDialog("CIBIL score must be between 300 and 900.");
        return 0;
    }
    if (a->totalAssets < 0) {
        showErrorDialog("Total Assets value cannot be negative.");
        return 0;
    }
    return 1;
}

gboolean drawRiskMeter(GtkWidget *widget, cairo_t *cr, gpointer data) {
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);
    double cx = width / 2.0;
    double cy = height - 14.0;
    double radius = fmin(width * 0.38, height * 0.78);
    double percent = currentDti / 100.0;
    double angle;
    double nx;
    double ny;

    if (percent < 0.0) percent = 0.0;
    if (percent > 1.0) percent = 1.0;

    cairo_set_line_width(cr, 14);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

    cairo_set_source_rgb(cr, 0.86, 0.90, 0.95);
    cairo_arc(cr, cx, cy, radius, M_PI, 2 * M_PI);
    cairo_stroke(cr);

    cairo_set_source_rgb(cr, 0.86, 0.15, 0.15); // Red (high risk / high DTI)
    cairo_arc(cr, cx, cy, radius, M_PI, M_PI + (M_PI * 0.35));
    cairo_stroke(cr);

    cairo_set_source_rgb(cr, 0.96, 0.62, 0.04); // Yellow (mod risk)
    cairo_arc(cr, cx, cy, radius, M_PI + (M_PI * 0.35), M_PI + (M_PI * 0.65));
    cairo_stroke(cr);

    cairo_set_source_rgb(cr, 0.13, 0.65, 0.32); // Green (low risk)
    cairo_arc(cr, cx, cy, radius, M_PI + (M_PI * 0.65), 2 * M_PI);
    cairo_stroke(cr);

    // DTI indicator needle
    angle = M_PI + (M_PI * percent);
    nx = cx + cos(angle) * (radius - 6);
    ny = cy + sin(angle) * (radius - 6);

    cairo_set_source_rgb(cr, 0.05, 0.18, 0.36);
    cairo_set_line_width(cr, 4);
    cairo_move_to(cr, cx, cy);
    cairo_line_to(cr, nx, ny);
    cairo_stroke(cr);

    cairo_arc(cr, cx, cy, 5, 0, 2 * M_PI);
    cairo_fill(cr);

    return FALSE;
}

void updateRiskText(double dti) {
    char text[80];
    if (dti <= 35)
        snprintf(text, sizeof(text), "DTI Risk Level: Low");
    else if (dti <= 50)
        snprintf(text, sizeof(text), "DTI Risk Level: Moderate");
    else
        snprintf(text, sizeof(text), "DTI Risk Level: High");

    gtk_label_set_text(GTK_LABEL(labelRiskLevel), text);
}

void resetResults(void) {
    currentDti = 0.0;
    gtk_label_set_text(GTK_LABEL(labelDecision), "Awaiting Analysis");
    gtk_label_set_text(GTK_LABEL(labelScore), "KNN Confidence: -- %");
    gtk_label_set_text(GTK_LABEL(labelEmi), "Estimated Monthly EMI: --");
    gtk_label_set_text(GTK_LABEL(labelDti), "Debt To Income Ratio: --");
    gtk_label_set_text(GTK_LABEL(labelReasons), "Key classification factors will appear after prediction.");
    gtk_label_set_text(GTK_LABEL(labelRiskLevel), "DTI Risk Level: --");
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(scoreProgress), 0.0);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(scoreProgress), "0%");
    setDecisionStyle("");
    setProgressStyle(0);
    gtk_widget_queue_draw(riskMeter);
}

void clearFields(GtkWidget *widget, gpointer data) {
    gtk_entry_set_text(GTK_ENTRY(entryName), "");
    gtk_entry_set_text(GTK_ENTRY(entryAge), "");
    gtk_entry_set_text(GTK_ENTRY(entryEmployment), "");
    gtk_entry_set_text(GTK_ENTRY(entryExistingEmi), "");
    gtk_entry_set_text(GTK_ENTRY(entryIncome), "");
    gtk_entry_set_text(GTK_ENTRY(entryLoan), "");
    gtk_entry_set_text(GTK_ENTRY(entryTenure), "");
    gtk_entry_set_text(GTK_ENTRY(entryCredit), "");
    gtk_entry_set_text(GTK_ENTRY(entryTotalAssets), "");
    resetResults();
}

void loadDemoApproved(GtkWidget *widget, gpointer data) {
    gtk_entry_set_text(GTK_ENTRY(entryName), "Alok Kumar (Approved Demo)");
    gtk_entry_set_text(GTK_ENTRY(entryAge), "35");           // Prime working age
    gtk_entry_set_text(GTK_ENTRY(entryEmployment), "12");
    gtk_entry_set_text(GTK_ENTRY(entryExistingEmi), "25000");
    gtk_entry_set_text(GTK_ENTRY(entryIncome), "8000000"); // 80 Lakhs Annual
    gtk_entry_set_text(GTK_ENTRY(entryLoan), "15000000");  // 1.5 Crore Loan
    gtk_entry_set_text(GTK_ENTRY(entryTenure), "20");      // 20 Years
    gtk_entry_set_text(GTK_ENTRY(entryCredit), "820");     // CIBIL Score (Excellent)
    gtk_entry_set_text(GTK_ENTRY(entryTotalAssets), "38000000"); // 3.8 Crore Total
}

void loadDemoRejected(GtkWidget *widget, gpointer data) {
    gtk_entry_set_text(GTK_ENTRY(entryName), "Vijay Mallya (Rejected Demo)");
    gtk_entry_set_text(GTK_ENTRY(entryAge), "52");           // Older, less years to repay
    gtk_entry_set_text(GTK_ENTRY(entryEmployment), "2");
    gtk_entry_set_text(GTK_ENTRY(entryExistingEmi), "110000");
    gtk_entry_set_text(GTK_ENTRY(entryIncome), "1500000"); // 15 Lakhs Annual
    gtk_entry_set_text(GTK_ENTRY(entryLoan), "35000000");  // 3.5 Crore Loan (Huge)
    gtk_entry_set_text(GTK_ENTRY(entryTenure), "10");      // 10 Years (Short term, high EMI)
    gtk_entry_set_text(GTK_ENTRY(entryCredit), "350");     // CIBIL Score (Terrible)
    gtk_entry_set_text(GTK_ENTRY(entryTotalAssets), "1700000"); // 17 Lakhs Total
}

void loadDemoReview(GtkWidget *widget, gpointer data) {
    gtk_entry_set_text(GTK_ENTRY(entryName), "Rahul Sharma (Review Demo)");
    gtk_entry_set_text(GTK_ENTRY(entryAge), "42");           // Middle-aged, borderline
    gtk_entry_set_text(GTK_ENTRY(entryEmployment), "7");
    gtk_entry_set_text(GTK_ENTRY(entryExistingEmi), "65000");
    gtk_entry_set_text(GTK_ENTRY(entryIncome), "5000000"); // 50 Lakhs Annual
    gtk_entry_set_text(GTK_ENTRY(entryLoan), "25000000");  // 2.5 Crore Loan (High)
    gtk_entry_set_text(GTK_ENTRY(entryTenure), "15");      // 15 Years (High EMI)
    gtk_entry_set_text(GTK_ENTRY(entryCredit), "710");     // CIBIL Score (Decent but not great)
    gtk_entry_set_text(GTK_ENTRY(entryTotalAssets), "19500000"); // 1.95 Crore Total
}

void checkLoan(GtkWidget *widget, gpointer data) {
    KaggleApplicant a;
    char name[128];
    char temp[512];
    char reasons[1024];
    double confidence = 0.0;

    readApplicantFromForm(&a, name, sizeof(name));

    if (!validateApplicant(&a, name))
        return;

    int knnDecision = evaluateKNN(&a, K_NEIGHBORS, &confidence, reasons);

    double emi = calculateEmi(a.loanAmount, a.loanTermYears);
    double monthlyIncome = a.incomeAnnum / 12.0;
    currentDti = ((emi + a.existingEmi) / monthlyIncome) * 100.0;

    int finalDecision = knnDecision;
    
    // Conditions for Review
    if (confidence <= 60.0) {
        finalDecision = 2; // Close call from KNN
    } else if (knnDecision == 1 && currentDti > 50.0) {
        finalDecision = 2; // Approved but high DTI
    } else if (knnDecision == 1 && a.cibilScore < 650) {
        finalDecision = 2; // Approved but low CIBIL
    } else if (knnDecision == 0 && a.cibilScore >= 750 && currentDti <= 35.0) {
        finalDecision = 2; // Rejected but excellent credit and low DTI
    }

    if (finalDecision == 1) {
        gtk_label_set_text(GTK_LABEL(labelDecision), "APPROVED");
        setDecisionStyle("APPROVED");
    } else if (finalDecision == 2) {
        gtk_label_set_text(GTK_LABEL(labelDecision), "REVIEW");
        setDecisionStyle("REVIEW");
    } else {
        gtk_label_set_text(GTK_LABEL(labelDecision), "REJECTED");
        setDecisionStyle("REJECTED");
    }

    snprintf(temp, sizeof(temp), "KNN Confidence: %.1f%%", confidence);
    gtk_label_set_text(GTK_LABEL(labelScore), temp);

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(scoreProgress), confidence / 100.0);
    snprintf(temp, sizeof(temp), "%.0f%%", confidence);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(scoreProgress), temp);
    setProgressStyle((int)confidence);

    snprintf(temp, sizeof(temp), "Estimated Monthly EMI: %.2f", emi);
    gtk_label_set_text(GTK_LABEL(labelEmi), temp);

    snprintf(temp, sizeof(temp), "Debt To Income Ratio: %.2f%%", currentDti);
    gtk_label_set_text(GTK_LABEL(labelDti), temp);

    gtk_label_set_text(GTK_LABEL(labelReasons), reasons);

    updateRiskText(currentDti);
    gtk_widget_queue_draw(riskMeter);

    saveLoanRecord(name, &a, finalDecision, confidence);
    writeSystemLog(name, finalDecision, confidence);
}

GtkWidget *iconLabel(const char *iconName, const char *text, const char *style) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *icon = gtk_image_new_from_icon_name(iconName, GTK_ICON_SIZE_MENU);
    GtkWidget *label = gtk_label_new(text);

    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    if (style != NULL)
        addStyle(label, style);

    gtk_box_pack_start(GTK_BOX(box), icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);

    return box;
}

GtkWidget *createSection(const char *title) {
    GtkWidget *frame = gtk_frame_new(NULL);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    GtkWidget *heading = gtk_label_new(title);

    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    addStyle(frame, "panel");
    addStyle(heading, "section-title");
    gtk_label_set_xalign(GTK_LABEL(heading), 0.0);

    gtk_container_add(GTK_CONTAINER(frame), box);
    gtk_box_pack_start(GTK_BOX(box), heading, FALSE, FALSE, 0);

    return frame;
}

GtkWidget *sectionBody(GtkWidget *section) {
    return gtk_bin_get_child(GTK_BIN(section));
}

void addInputRow(GtkWidget *grid,
                 int row,
                 const char *iconName,
                 const char *labelText,
                 GtkWidget **entry) {
    GtkWidget *label = iconLabel(iconName, labelText, "field-label");

    *entry = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(*entry), 18);
    gtk_widget_set_hexpand(*entry, TRUE);

    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), *entry, 1, row, 1, 1);
}

void addComboRow(GtkWidget *grid,
                 int row,
                 const char *iconName,
                 const char *labelText,
                 GtkWidget *combo) {
    GtkWidget *label = iconLabel(iconName, labelText, "field-label");
    gtk_widget_set_hexpand(combo, TRUE);

    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), combo, 1, row, 1, 1);
}

GtkWidget *createIconButton(const char *iconName,
                            const char *text,
                            const char *style) {
    GtkWidget *button = gtk_button_new();
    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *icon = gtk_image_new_from_icon_name(iconName, GTK_ICON_SIZE_BUTTON);
    GtkWidget *label = gtk_label_new(text);

    gtk_box_pack_start(GTK_BOX(content), icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), label, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(button), content);
    addStyle(button, style);

    return button;
}

GtkWidget *buildHeader(void) {
    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *title = gtk_label_new("Loan Evaluation System");
    GtkWidget *subtitle =
        gtk_label_new("Data-driven predictions using K-Nearest Neighbors (KNN) model implemented in pure C");

    addStyle(header, "header");
    addStyle(title, "title");
    addStyle(subtitle, "subtitle");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_label_set_xalign(GTK_LABEL(subtitle), 0.0);

    gtk_box_pack_start(GTK_BOX(header), title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(header), subtitle, FALSE, FALSE, 0);

    return header;
}

GtkWidget *buildApplicantSection(void) {
    GtkWidget *section = createSection("Applicant Profiling");
    GtkWidget *grid = gtk_grid_new();

    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);
    gtk_box_pack_start(GTK_BOX(sectionBody(section)), grid, FALSE, FALSE, 0);

    addInputRow(grid, 0, "avatar-default", "Applicant Name",       &entryName);
    addInputRow(grid, 1, "emblem-favorite", "Applicant Age (Years)", &entryAge);
    addInputRow(grid, 2, "appointment-new", "Employment Years",      &entryEmployment);
    addInputRow(grid, 3, "go-home",         "Existing Monthly EMI",  &entryExistingEmi);

    return section;
}

GtkWidget *buildFinancialSection(void) {
    GtkWidget *section = createSection("Financial Details & Assets");
    GtkWidget *grid = gtk_grid_new();

    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);
    gtk_box_pack_start(GTK_BOX(sectionBody(section)), grid, FALSE, FALSE, 0);

    addInputRow(grid, 0, "go-home", "Annual Income", &entryIncome);
    addInputRow(grid, 1, "document-open", "Loan Amount", &entryLoan);
    addInputRow(grid, 2, "view-refresh", "Loan Tenure (Years)", &entryTenure);
    addInputRow(grid, 3, "emblem-important", "CIBIL (Credit) Score", &entryCredit);
    
    addInputRow(grid, 4, "folder", "Total Assets Value", &entryTotalAssets);

    return section;
}

GtkWidget *buildButtonRow(void) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *btnCheck =
        createIconButton("system-search", "Evaluate KNN", "primary-button");
    GtkWidget *btnClear =
        createIconButton("edit-clear", "Clear Fields", "secondary-button");
    GtkWidget *btnDemoApp =
        createIconButton("document-new", "Demo (Approved)", "demo-button");
    GtkWidget *btnDemoRev =
        createIconButton("document-new", "Demo (Review)", "demo-button");
    GtkWidget *btnDemoRej =
        createIconButton("document-new", "Demo (Rejected)", "demo-button");

    gtk_box_pack_start(GTK_BOX(row), btnCheck, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), btnClear, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), btnDemoApp, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), btnDemoRev, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), btnDemoRej, FALSE, FALSE, 0);

    g_signal_connect(btnCheck, "clicked", G_CALLBACK(checkLoan), NULL);
    g_signal_connect(btnClear, "clicked", G_CALLBACK(clearFields), NULL);
    g_signal_connect(btnDemoApp, "clicked", G_CALLBACK(loadDemoApproved), NULL);
    g_signal_connect(btnDemoRev, "clicked", G_CALLBACK(loadDemoReview), NULL);
    g_signal_connect(btnDemoRej, "clicked", G_CALLBACK(loadDemoRejected), NULL);

    return row;
}

GtkWidget *buildResultSection(void) {
    GtkWidget *section = createSection("KNN Model Prediction");
    GtkWidget *body = sectionBody(section);
    GtkWidget *metricGrid = gtk_grid_new();
    GtkWidget *factorsTitle;

    decisionBox = gtk_event_box_new();
    labelDecision = gtk_label_new("Awaiting Analysis");
    addStyle(decisionBox, "result-box");
    addStyle(decisionBox, "decision-default");
    addStyle(labelDecision, "decision-text");
    gtk_container_add(GTK_CONTAINER(decisionBox), labelDecision);

    labelScore = gtk_label_new("KNN Confidence: -- %");
    labelEmi = gtk_label_new("Estimated Monthly EMI: --");
    labelDti = gtk_label_new("Debt To Income Ratio: --");
    labelRiskLevel = gtk_label_new("DTI Risk Level: --");
    labelReasons = gtk_label_new("Key classification factors will appear after prediction.");

    gtk_label_set_xalign(GTK_LABEL(labelScore), 0.0);
    gtk_label_set_xalign(GTK_LABEL(labelEmi), 0.0);
    gtk_label_set_xalign(GTK_LABEL(labelDti), 0.0);
    gtk_label_set_xalign(GTK_LABEL(labelRiskLevel), 0.0);
    gtk_label_set_xalign(GTK_LABEL(labelReasons), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(labelReasons), TRUE);

    addStyle(labelScore, "metric-label");
    addStyle(labelEmi, "metric-label");
    addStyle(labelDti, "metric-label");
    addStyle(labelRiskLevel, "metric-label");
    addStyle(labelReasons, "factors");

    scoreProgress = gtk_progress_bar_new();
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(scoreProgress), TRUE);

    riskMeter = gtk_drawing_area_new();
    gtk_widget_set_size_request(riskMeter, 260, 130);
    g_signal_connect(riskMeter, "draw", G_CALLBACK(drawRiskMeter), NULL);

    gtk_grid_set_row_spacing(GTK_GRID(metricGrid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(metricGrid), 12);

    gtk_grid_attach(GTK_GRID(metricGrid),
                    iconLabel("emblem-important", "Prediction", "metric-label"),
                    0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(metricGrid), decisionBox, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(metricGrid),
                    iconLabel("view-statistics", "Confidence", "metric-label"),
                    0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(metricGrid), labelScore, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(metricGrid), scoreProgress, 1, 2, 1, 1);
    
    gtk_grid_attach(GTK_GRID(metricGrid),
                    iconLabel("accessories-calculator", "EMI Calc", "metric-label"),
                    0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(metricGrid), labelEmi, 1, 3, 1, 1);
    
    gtk_grid_attach(GTK_GRID(metricGrid),
                    iconLabel("dialog-warning", "DTI Calc", "metric-label"),
                    0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(metricGrid), labelDti, 1, 4, 1, 1);

    factorsTitle = iconLabel("format-justify-left", "Nearest Neighbor Data Points", "section-title");

    gtk_box_pack_start(GTK_BOX(body), metricGrid, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(body), riskMeter, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(body), labelRiskLevel, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(body), factorsTitle, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(body), labelReasons, FALSE, FALSE, 0);

    return section;
}

int main(int argc, char *argv[]) {
    GtkWidget *window;
    GtkWidget *mainBox;
    GtkWidget *content;
    GtkWidget *leftColumn;
    GtkWidget *rightColumn;

    gtk_init(&argc, &argv);
    loadCss();

    // Load dataset - always from the exe's own directory
    char datasetPath[MAX_PATH];
    buildExePath(datasetPath, sizeof(datasetPath), "loan_approval_dataset.csv");
    int loaded = loadDataset(datasetPath);
    
    if (loaded > 0) {
        printf("Successfully loaded %d records from Kaggle CSV.\n", loaded);
    } else {
        g_printerr("CRITICAL ERROR: Failed to load loan_approval_dataset.csv from: %s\n", datasetPath);
    }

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Loan Evaluation System");
    gtk_window_set_default_size(GTK_WINDOW(window), 1020, 800);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 18);
    leftColumn = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    rightColumn = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    gtk_container_add(GTK_CONTAINER(window), mainBox);
    gtk_box_pack_start(GTK_BOX(mainBox), buildHeader(), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(mainBox), content, TRUE, TRUE, 18);

    gtk_widget_set_margin_start(content, 18);
    gtk_widget_set_margin_end(content, 18);
    gtk_widget_set_margin_bottom(content, 18);

    gtk_box_pack_start(GTK_BOX(content), leftColumn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(content), rightColumn, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(leftColumn), buildApplicantSection(), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(leftColumn), buildFinancialSection(), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(leftColumn), buildButtonRow(), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(rightColumn), buildResultSection(), TRUE, TRUE, 0);

    resetResults();
    
    if (loaded == 0) {
        showErrorDialog("Kaggle loan_approval_dataset.csv could not be found! Predictions will not work.");
    }
    
    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
