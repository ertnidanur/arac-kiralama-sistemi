#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define CAR_FILE "car.txt"
#define CUSTOMER_FILE "customer.txt"
#define RENTAL_FILE "rental.txt"

#define BRAND_LEN 20
#define MODEL_LEN 20
#define ID_LEN 12
#define NAME_LEN 20
#define PHONE_LEN 15
#define DATE_LEN 20

typedef struct {
    char brand[BRAND_LEN];
    char model[MODEL_LEN];
    int ID;
    int year;
    int status;
} Car;

typedef struct {
    char ID[ID_LEN];
    char name[NAME_LEN];
    char surname[NAME_LEN];
    char phone[PHONE_LEN];
} Customer;

typedef struct {
    int CarID;
    char CustomerID[ID_LEN];
    char rentDate[DATE_LEN];
    char returnDate[DATE_LEN];
    int returned; 
} Rental;

static Car *ArabaArray = NULL;
static int ArabaSayisi = 0;
static int girdiBitti = 0; 

static void SatirSonunuTemizle(void) {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {
    }
}

static int MetinOku(const char *mesaj, char *hedef, size_t boyut) {
    char satir[256]; 
    size_t uzunluk, i;

    printf("%s", mesaj);
    hedef[0] = '\0';
    if (fgets(satir, sizeof(satir), stdin) == NULL) {
        girdiBitti = 1;
        return 0;
    }

    uzunluk = strlen(satir);
    if (uzunluk > 0 && satir[uzunluk - 1] == '\n') {
        satir[--uzunluk] = '\0';
        if (uzunluk > 0 && satir[uzunluk - 1] == '\r') {
            satir[--uzunluk] = '\0';
        }
    } else if (!feof(stdin)) {
        SatirSonunuTemizle();
        printf("Girdi cok uzun. Lutfen tekrar deneyin.\n");
        return 0;
    }

    if (uzunluk == 0) {
        printf("Bu alan bos birakilamaz.\n");
        return 0;
    }
    if (uzunluk >= boyut) {
        printf("Girdi cok uzun (en fazla %u karakter).\n", (unsigned)(boyut - 1));
        return 0;
    }
    for (i = 0; i < uzunluk; i++) {
        if (isspace((unsigned char)satir[i])) {
            printf("Bosluk kullanmayin (ornek: Land_Rover).\n");
            return 0;
        }
    }
    memcpy(hedef, satir, uzunluk + 1);
    return 1;
}

static int SayiOku(const char *mesaj, int *sayi) {
    char satir[64];
    char *son;
    long deger;

    if (!MetinOku(mesaj, satir, sizeof(satir))) {
        return 0;
    }

    deger = strtol(satir, &son, 10);
    if (son == satir || *son != '\0' || deger < -2147483647L - 1 ||
        deger > 2147483647L) {
        printf("Gecersiz sayi girdiniz.\n");
        return 0;
    }

    *sayi = (int)deger;
    return 1;
}

static int YilGecerliMi(int yil) {
    return yil >= 1886 && yil <= 2100;
}

static int TCGecerliMi(const char *id) {
    int d[11], i, tek, cift, toplam;

    if (strlen(id) != 11) {
        return 0;
    }
    for (i = 0; i < 11; i++) {
        if (!isdigit((unsigned char)id[i])) {
            return 0;
        }
        d[i] = id[i] - '0';
    }
    if (d[0] == 0) {
        return 0;
    }

    tek = d[0] + d[2] + d[4] + d[6] + d[8];
    cift = d[1] + d[3] + d[5] + d[7];
    if ((((tek * 7 - cift) % 10) + 10) % 10 != d[9]) {
        return 0;
    }

    toplam = 0;
    for (i = 0; i < 10; i++) {
        toplam += d[i];
    }
    return toplam % 10 == d[10];
}

static int TelefonGecerliMi(const char *tel) {
    int i = 0, rakam = 0;

    if (tel[0] == '+') {
        i = 1;
    }
    for (; tel[i] != '\0'; i++) {
        if (!isdigit((unsigned char)tel[i])) {
            return 0;
        }
        rakam++;
    }
    return rakam >= 10 && rakam <= 13;
}

static int TarihGecerliMi(const char *t) {
    static const int gun[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int y, m, d, maxGun, i;

    if (strlen(t) != 10 || t[4] != '-' || t[7] != '-') {
        return 0;
    }
    for (i = 0; i < 10; i++) {
        if (i != 4 && i != 7 && !isdigit((unsigned char)t[i])) {
            return 0;
        }
    }
    if (sscanf(t, "%d-%d-%d", &y, &m, &d) != 3) {
        return 0;
    }
    if (y < 2000 || y > 2100 || m < 1 || m > 12) {
        return 0;
    }
    maxGun = gun[m - 1];
    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) {
        maxGun = 29;
    }
    return d >= 1 && d <= maxGun;
}

static int TarihOku(const char *mesaj, char *hedef, size_t boyut) {
    if (!MetinOku(mesaj, hedef, boyut)) {
        return 0;
    }
    if (!TarihGecerliMi(hedef)) {
        printf("Gecersiz tarih. Format: YYYY-MM-DD (ornek: 2026-10-09).\n");
        return 0;
    }
    return 1;
}

static int DosyaDegistir(const char *gecici, const char *hedef) {
    if (rename(gecici, hedef) == 0) {
        return 1;
    }
    remove(hedef);
    if (rename(gecici, hedef) == 0) {
        return 1;
    }
    printf("Dosya guncellenemedi: %s\n", hedef);
    return 0;
}

static int AraclariYukle(void) {
    FILE *fptr;
    Car car;
    Car *gecici;

    free(ArabaArray);
    ArabaArray = NULL;
    ArabaSayisi = 0;

    fptr = fopen(CAR_FILE, "r");
    if (fptr == NULL) {
        return 1; 
    }
    while (fscanf(fptr, "%19s %19s %d %d %d",
                  car.brand, car.model, &car.ID, &car.year, &car.status) == 5) {
        gecici = realloc(ArabaArray, (size_t)(ArabaSayisi + 1) * sizeof(Car));
        if (gecici == NULL) {
            fclose(fptr);
            return 0;
        }
        ArabaArray = gecici;
        ArabaArray[ArabaSayisi++] = car;
    }

    if (ferror(fptr)) {
        fclose(fptr);
        return 0;
    }
    fclose(fptr);
    return 1;
}

static int AracKaydet(void) {
    FILE *fptr;
    int i;

    fptr = fopen(CAR_FILE ".tmp", "w");
    if (fptr == NULL) {
        printf("Arac dosyasi yazmak icin acilamadi.\n");
        return 0;
    }
    for (i = 0; i < ArabaSayisi; i++) {
        if (fprintf(fptr, "%s %s %d %d %d\n",
                    ArabaArray[i].brand, ArabaArray[i].model,
                    ArabaArray[i].ID, ArabaArray[i].year,
                    ArabaArray[i].status) < 0) {
            fclose(fptr);
            remove(CAR_FILE ".tmp");
            printf("Arac dosyasina yazilamadi.\n");
            return 0;
        }
    }
    if (fclose(fptr) != 0) {
        printf("Arac dosyasi kapatilirken hata olustu.\n");
        return 0;
    }
    return DosyaDegistir(CAR_FILE ".tmp", CAR_FILE);
}

static int AracIndexBul(int id) {
    int i;
    for (i = 0; i < ArabaSayisi; i++) {
        if (ArabaArray[i].ID == id) {
            return i;
        }
    }
    return -1;
}

static int AracIDVarMi(int id) {
    return AracIndexBul(id) != -1;
}

static int MusterileriYukle(Customer **liste, int *adet) {
    FILE *fptr;
    Customer c;
    Customer *gecici;

    *liste = NULL;
    *adet = 0;

    fptr = fopen(CUSTOMER_FILE, "r");
    if (fptr == NULL) {
        return 1;
    }
    while (fscanf(fptr, "%11s %19s %19s %14s",
                  c.ID, c.name, c.surname, c.phone) == 4) {
        gecici = realloc(*liste, (size_t)(*adet + 1) * sizeof(Customer));
        if (gecici == NULL) {
            free(*liste);
            *liste = NULL;
            *adet = 0;
            fclose(fptr);
            return 0;
        }
        *liste = gecici;
        (*liste)[(*adet)++] = c;
    }
    fclose(fptr);
    return 1;
}

static int MusterileriKaydet(const Customer *liste, int adet) {
    FILE *fptr;
    int i;

    fptr = fopen(CUSTOMER_FILE ".tmp", "w");
    if (fptr == NULL) {
        printf("Musteri dosyasi yazmak icin acilamadi.\n");
        return 0;
    }
    for (i = 0; i < adet; i++) {
        if (fprintf(fptr, "%s %s %s %s\n", liste[i].ID, liste[i].name,
                    liste[i].surname, liste[i].phone) < 0) {
            fclose(fptr);
            remove(CUSTOMER_FILE ".tmp");
            printf("Musteri dosyasina yazilamadi.\n");
            return 0;
        }
    }
    if (fclose(fptr) != 0) {
        printf("Musteri dosyasi kapatilirken hata olustu.\n");
        return 0;
    }
    return DosyaDegistir(CUSTOMER_FILE ".tmp", CUSTOMER_FILE);
}

static int MusteriIndexBul(const Customer *liste, int adet, const char *id) {
    int i;
    for (i = 0; i < adet; i++) {
        if (strcmp(liste[i].ID, id) == 0) {
            return i;
        }
    }
    return -1;
}

static int KiralamalariYukle(Rental **liste, int *adet) {
    FILE *fptr;
    Rental r;
    Rental *gecici;

    *liste = NULL;
    *adet = 0;

    fptr = fopen(RENTAL_FILE, "r");
    if (fptr == NULL) {
        return 1;
    }
    while (fscanf(fptr, "%d %11s %19s %19s %d",
                  &r.CarID, r.CustomerID, r.rentDate, r.returnDate,
                  &r.returned) == 5) {
        gecici = realloc(*liste, (size_t)(*adet + 1) * sizeof(Rental));
        if (gecici == NULL) {
            free(*liste);
            *liste = NULL;
            *adet = 0;
            fclose(fptr);
            return 0;
        }
        *liste = gecici;
        (*liste)[(*adet)++] = r;
    }
    fclose(fptr);
    return 1;
}

static int KiralamalariKaydet(const Rental *liste, int adet) {
    FILE *fptr;
    int i;

    fptr = fopen(RENTAL_FILE ".tmp", "w");
    if (fptr == NULL) {
        printf("Kiralama dosyasi yazmak icin acilamadi.\n");
        return 0;
    }
    for (i = 0; i < adet; i++) {
        if (fprintf(fptr, "%d %s %s %s %d\n", liste[i].CarID,
                    liste[i].CustomerID, liste[i].rentDate,
                    liste[i].returnDate, liste[i].returned) < 0) {
            fclose(fptr);
            remove(RENTAL_FILE ".tmp");
            printf("Kiralama dosyasina yazilamadi.\n");
            return 0;
        }
    }
    if (fclose(fptr) != 0) {
        printf("Kiralama dosyasi kapatilirken hata olustu.\n");
        return 0;
    }
    return DosyaDegistir(RENTAL_FILE ".tmp", RENTAL_FILE);
}


static void AracYazdir(const Car *c) {
    printf("Marka: %s | Model: %s | ID: %d | Yil: %d | Durum: %s\n",
           c->brand, c->model, c->ID, c->year,
           c->status == 1 ? "Musait" : "Kirada");
}

static void AracEkle(void) {
    Car car;
    Car *gecici;

    if (!MetinOku("Aracin markasi: ", car.brand, sizeof(car.brand)) ||
        !MetinOku("Aracin modeli: ", car.model, sizeof(car.model))) {
        return;
    }
    if (!SayiOku("Arac ID: ", &car.ID) ||
        !SayiOku("Aracin uretim yili: ", &car.year)) {
        return;
    }

    if (car.ID <= 0) {
        printf("Arac ID pozitif olmalidir.\n");
        return;
    }
    if (!YilGecerliMi(car.year)) {
        printf("Gecerli bir uretim yili giriniz.\n");
        return;
    }
    if (AracIDVarMi(car.ID)) {
        printf("Bu ID ile kayitli bir arac zaten var.\n");
        return;
    }

    car.status = 1;
    gecici = realloc(ArabaArray, (size_t)(ArabaSayisi + 1) * sizeof(Car));
    if (gecici == NULL) {
        printf("Bellek ayrilamadi; arac eklenemedi.\n");
        return;
    }
    ArabaArray = gecici;
    ArabaArray[ArabaSayisi++] = car;

    if (AracKaydet()) {
        printf("Arac basariyla eklendi.\n");
    }
}

static void AracSil(void) {
    int id, index, i;

    if (!SayiOku("Silinecek aracin ID'sini giriniz: ", &id)) {
        return;
    }
    index = AracIndexBul(id);
    if (index == -1) {
        printf("Arac bulunamadi.\n");
        return;
    }
    if (ArabaArray[index].status == 0) {
        printf("Kirada olan arac silinemez. Once araci teslim alin.\n");
        return;
    }

    for (i = index; i < ArabaSayisi - 1; i++) {
        ArabaArray[i] = ArabaArray[i + 1];
    }
    ArabaSayisi--;

    if (ArabaSayisi == 0) {
        free(ArabaArray);
        ArabaArray = NULL;
    } else {
        Car *gecici = realloc(ArabaArray, (size_t)ArabaSayisi * sizeof(Car));
        if (gecici != NULL) {
            ArabaArray = gecici;
        }
    }

    if (AracKaydet()) {
        printf("Arac basariyla silindi (gecmis kiralama kayitlari korunur).\n");
    }
}

static void AracGuncelle(void) {
    int id, index, yil;
    char marka[BRAND_LEN], model[MODEL_LEN];

    if (!SayiOku("Guncellenecek aracin ID'sini giriniz: ", &id)) {
        return;
    }
    index = AracIndexBul(id);
    if (index == -1) {
        printf("Arac bulunamadi.\n");
        return;
    }

    if (!MetinOku("Yeni marka: ", marka, sizeof(marka)) ||
        !MetinOku("Yeni model: ", model, sizeof(model)) ||
        !SayiOku("Yeni uretim yili: ", &yil)) {
        return;
    }
    if (!YilGecerliMi(yil)) {
        printf("Gecerli bir uretim yili giriniz.\n");
        return;
    }

    snprintf(ArabaArray[index].brand, sizeof(ArabaArray[index].brand), "%s", marka);
    snprintf(ArabaArray[index].model, sizeof(ArabaArray[index].model), "%s", model);
    ArabaArray[index].year = yil;

    if (AracKaydet()) {
        printf("Arac bilgileri guncellendi.\n");
    }
}

static void AracArama(void) {
    int id, index;

    if (!SayiOku("Aramak istediginiz aracin ID'sini giriniz: ", &id)) {
        return;
    }
    index = AracIndexBul(id);
    if (index == -1) {
        printf("Arac bulunamadi.\n");
        return;
    }
    AracYazdir(&ArabaArray[index]);
}

static void AraclariListele(void) {
    int i;

    if (ArabaSayisi == 0) {
        printf("Kayitli arac bulunmuyor.\n");
        return;
    }
    printf("\n--- Arac Listesi ---\n");
    for (i = 0; i < ArabaSayisi; i++) {
        AracYazdir(&ArabaArray[i]);
    }
}


static void MusteriEkle(void) {
    Customer c;
    Customer *liste;
    int adet;
    FILE *fptr;

    if (!MetinOku("Musteri TC (11 rakam): ", c.ID, sizeof(c.ID))) {
        return;
    }
    if (!TCGecerliMi(c.ID)) {
        printf("Gecersiz TC kimlik numarasi.\n");
        return;
    }

    if (!MusterileriYukle(&liste, &adet)) {
        printf("Musteri dosyasi okunamadi.\n");
        return;
    }
    if (MusteriIndexBul(liste, adet, c.ID) != -1) {
        free(liste);
        printf("Bu TC ile kayitli bir musteri zaten var.\n");
        return;
    }
    free(liste);

    if (!MetinOku("Ad: ", c.name, sizeof(c.name)) ||
        !MetinOku("Soyad: ", c.surname, sizeof(c.surname)) ||
        !MetinOku("Telefon (or. 05551234567): ", c.phone, sizeof(c.phone))) {
        return;
    }
    if (!TelefonGecerliMi(c.phone)) {
        printf("Gecersiz telefon. 10-13 rakam girin (basta + olabilir).\n");
        return;
    }

    fptr = fopen(CUSTOMER_FILE, "a");
    if (fptr == NULL) {
        printf("Musteri dosyasi acilamadi.\n");
        return;
    }
    if (fprintf(fptr, "%s %s %s %s\n",
                c.ID, c.name, c.surname, c.phone) < 0) {
        printf("Musteri kaydedilemedi.\n");
        fclose(fptr);
        return;
    }
    fclose(fptr);
    printf("Musteri basariyla eklendi.\n");
}

static void MusteriSil(void) {
    char id[ID_LEN];
    Customer *musteriler;
    Rental *kiralamalar;
    int musteriSayisi, kiralamaSayisi, index, i;

    if (!MetinOku("Silinecek musterinin TC'si: ", id, sizeof(id))) {
        return;
    }
    if (!MusterileriYukle(&musteriler, &musteriSayisi)) {
        printf("Musteri dosyasi okunamadi.\n");
        return;
    }
    index = MusteriIndexBul(musteriler, musteriSayisi, id);
    if (index == -1) {
        free(musteriler);
        printf("Musteri bulunamadi.\n");
        return;
    }

    if (!KiralamalariYukle(&kiralamalar, &kiralamaSayisi)) {
        free(musteriler);
        printf("Kiralama dosyasi okunamadi.\n");
        return;
    }
    for (i = 0; i < kiralamaSayisi; i++) {
        if (kiralamalar[i].returned == 0 &&
            strcmp(kiralamalar[i].CustomerID, id) == 0) {
            printf("Uzerinde kirada arac olan musteri silinemez.\n");
            free(kiralamalar);
            free(musteriler);
            return;
        }
    }
    free(kiralamalar);

    for (i = index; i < musteriSayisi - 1; i++) {
        musteriler[i] = musteriler[i + 1];
    }
    musteriSayisi--;

    if (MusterileriKaydet(musteriler, musteriSayisi)) {
        printf("Musteri silindi.\n");
    }
    free(musteriler);
}

static void MusteriAra(void) {
    char arananID[ID_LEN];
    Customer *liste;
    int adet, index;

    if (!MetinOku("Musteri TC'sini giriniz: ", arananID, sizeof(arananID))) {
        return;
    }
    if (!MusterileriYukle(&liste, &adet)) {
        printf("Musteri dosyasi okunamadi.\n");
        return;
    }
    index = MusteriIndexBul(liste, adet, arananID);
    if (index == -1) {
        printf("Musteri bulunamadi.\n");
    } else {
        printf("Musteri: %s %s %s %s\n", liste[index].ID, liste[index].name,
               liste[index].surname, liste[index].phone);
    }
    free(liste);
}

static void MusterileriListele(void) {
    Customer *liste;
    int adet, i;

    if (!MusterileriYukle(&liste, &adet)) {
        printf("Musteri dosyasi okunamadi.\n");
        return;
    }
    if (adet == 0) {
        printf("Kayitli musteri bulunmuyor.\n");
        free(liste);
        return;
    }
    printf("\n--- Musteri Listesi ---\n");
    for (i = 0; i < adet; i++) {
        printf("%s %s %s %s\n", liste[i].ID, liste[i].name,
               liste[i].surname, liste[i].phone);
    }
    free(liste);
}

static void AracKiralama(void) {
    int carID, index, musteriSayisi, musteriIndex;
    char customerID[ID_LEN];
    Customer *musteriler;
    Rental r;
    FILE *fptr;

    if (!SayiOku("Kiralamak istediginiz aracin ID'sini giriniz: ", &carID)) {
        return;
    }
    index = AracIndexBul(carID);
    if (index == -1) {
        printf("Arac bulunamadi.\n");
        return;
    }
    if (ArabaArray[index].status == 0) {
        printf("Bu arac su an kirada.\n");
        return;
    }

    if (!MetinOku("Musteri TC (11 rakam): ", customerID, sizeof(customerID))) {
        return;
    }
    if (!MusterileriYukle(&musteriler, &musteriSayisi)) {
        printf("Musteri dosyasi okunamadi.\n");
        return;
    }
    musteriIndex = MusteriIndexBul(musteriler, musteriSayisi, customerID);
    free(musteriler);
    if (musteriIndex == -1) {
        printf("Bu TC ile kayitli musteri bulunamadi. Once musteri ekleyin.\n");
        return;
    }

    if (!TarihOku("Kiralama tarihi (YYYY-MM-DD): ", r.rentDate, sizeof(r.rentDate)) ||
        !TarihOku("Teslim tarihi (YYYY-MM-DD): ", r.returnDate, sizeof(r.returnDate))) {
        return;
    }
    if (strcmp(r.returnDate, r.rentDate) < 0) {
        printf("Teslim tarihi kiralama tarihinden once olamaz.\n");
        return;
    }

    r.CarID = carID;
    snprintf(r.CustomerID, sizeof(r.CustomerID), "%s", customerID);
    r.returned = 0;

    fptr = fopen(RENTAL_FILE, "a");
    if (fptr == NULL) {
        printf("Kiralama dosyasi acilamadi.\n");
        return;
    }
    if (fprintf(fptr, "%d %s %s %s %d\n", r.CarID, r.CustomerID,
                r.rentDate, r.returnDate, r.returned) < 0) {
        fclose(fptr);
        printf("Kiralama kaydi yazilamadi.\n");
        return;
    }
    fclose(fptr);

    ArabaArray[index].status = 0;
    if (!AracKaydet()) {
        printf("Uyari: Kiralama kaydedildi ancak arac durumu dosyaya yazilamadi.\n");
        return;
    }
    printf("Arac basariyla kiralandi.\n");
}

static void AracTeslim(void) {
    int carID, index, i, bulunan = -1, kiralamaSayisi;
    Rental *kiralamalar;

    if (!SayiOku("Teslim alinacak aracin ID'sini giriniz: ", &carID)) {
        return;
    }
    index = AracIndexBul(carID);
    if (index == -1) {
        printf("Arac bulunamadi.\n");
        return;
    }
    if (ArabaArray[index].status == 1) {
        printf("Bu arac zaten musait; kirada degil.\n");
        return;
    }

    if (!KiralamalariYukle(&kiralamalar, &kiralamaSayisi)) {
        printf("Kiralama dosyasi okunamadi.\n");
        return;
    }
    for (i = kiralamaSayisi - 1; i >= 0; i--) { 
        if (kiralamalar[i].CarID == carID && kiralamalar[i].returned == 0) {
            bulunan = i;
            break;
        }
    }
    if (bulunan != -1) {
        kiralamalar[bulunan].returned = 1;
        if (!KiralamalariKaydet(kiralamalar, kiralamaSayisi)) {
            free(kiralamalar);
            return;
        }
    } else {
        printf("Uyari: Bu arac icin acik kiralama kaydi bulunamadi.\n");
    }
    free(kiralamalar);

    ArabaArray[index].status = 1;
    if (AracKaydet()) {
        printf("Arac teslim alindi.\n");
    }
}

static void KiralamaGecmisi(void) {
    Rental *liste;
    int adet, i;

    if (!KiralamalariYukle(&liste, &adet)) {
        printf("Kiralama dosyasi okunamadi.\n");
        return;
    }
    if (adet == 0) {
        printf("Kiralama gecmisi bulunmuyor.\n");
        free(liste);
        return;
    }

    printf("\n--- Kiralama Gecmisi ---\n");
    for (i = 0; i < adet; i++) {
        printf("Arac ID: %d%s\nMusteri: %s\nKiralama: %s\nTeslim: %s\nDurum: %s\n\n",
               liste[i].CarID,
               AracIDVarMi(liste[i].CarID) ? "" : " (silinmis arac)",
               liste[i].CustomerID, liste[i].rentDate, liste[i].returnDate,
               liste[i].returned ? "Iade edildi" : "Kirada");
    }
    free(liste);
}

static void Menu(void) {
    int choice;

    while (!girdiBitti) {
        printf("\n=== ARAC KIRALAMA SISTEMI ===\n");
        printf("1.  Arac ekle\n");
        printf("2.  Arac sil\n");
        printf("3.  Arac guncelle\n");
        printf("4.  Arac ara\n");
        printf("5.  Araclari listele\n");
        printf("6.  Musteri ekle\n");
        printf("7.  Musteri sil\n");
        printf("8.  Musteri ara\n");
        printf("9.  Musterileri listele\n");
        printf("10. Arac kirala\n");
        printf("11. Arac teslim al\n");
        printf("12. Kiralama gecmisi\n");
        printf("0.  Cikis\n");

        if (!SayiOku("Seciminiz: ", &choice)) {
            continue;
        }

        switch (choice) {
            case 1:  AracEkle(); break;
            case 2:  AracSil(); break;
            case 3:  AracGuncelle(); break;
            case 4:  AracArama(); break;
            case 5:  AraclariListele(); break;
            case 6:  MusteriEkle(); break;
            case 7:  MusteriSil(); break;
            case 8:  MusteriAra(); break;
            case 9:  MusterileriListele(); break;
            case 10: AracKiralama(); break;
            case 11: AracTeslim(); break;
            case 12: KiralamaGecmisi(); break;
            case 0:  return;
            default: printf("Gecersiz secim!\n");
        }
    }
    printf("\nGirdi sonlandi, programdan cikiliyor.\n");
}

int main(void) {
    if (!AraclariYukle()) {
        printf("Arac dosyasi okunurken hata olustu.\n");
    }
    Menu();
    free(ArabaArray);
    return 0;
}
