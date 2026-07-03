#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QRandomGenerator>
#include <QPropertyAnimation>
#include <QRegularExpression>
#include <cmath>
#include <QMessageBox>
#include <QSoundEffect>
#include <QUrl>
#include <QtTextToSpeech/QTextToSpeech>
#include <QStandardPaths>
#include <QVoice>
#include <QPainter>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , highlightedLabel(nullptr)
    , currentIndex(0)
    , currentWordIndex(0)
    , rhymeRunning(false)
{
    ui->setupUi(this);

    QMessageBox aboutBox(this);
    aboutBox.setWindowTitle("Справка");
    aboutBox.setText("Приложение разработала\n\nАндрейковец Яна");
    aboutBox.exec();

    setFixedSize(800, 600);

    // 1. Озвучка
    speech = new QTextToSpeech(this);
    auto voices = speech->availableVoices();
    for (const QVoice &voice : voices) {
        if (voice.locale().language() == QLocale::Russian) {
            speech->setVoice(voice);
            break;
        }
    }
    speech->setVolume(1.0);
    speech->setRate(0.75);

    // 2. Пути (Ищем папки с текстами и фото строго рядом с .exe)
    basePath = QCoreApplication::applicationDirPath() + "/";

    // Загружаем тексты и фото из внешних папок рядом с .exe
    allRhymes = loadRhymesFromFolder(basePath + "texts/");
    persons = loadPhotos(basePath + "photos/");

    if (allRhymes.isEmpty() || persons.isEmpty()) {
        QString errorMsg = QString("Ошибка загрузки!\n\n") +
                           "Проверьте, что рядом с .exe файлом созданы папки:\n" +
                           "1. Папка 'texts' с файлами .txt\n" +
                           "2. Папка 'photos' с картинками участников.";
        QMessageBox::critical(this, "Ошибка", errorMsg);
    } else {
        currentIndex = QRandomGenerator::global()->bounded(persons.size());
        displayPhotosInCircle(persons);
    }

    // 3. Интерфейс
    ui->nextWordButton->setGeometry(300, 250, 200, 45);
    ui->nextWordButton->setText("ЗАПУСТИТЬ");

    ui->longestWordLabel->setStyleSheet("background: rgba(255,255,255,180); border-radius: 10px; font-weight: bold; color: black;");
    ui->longestWordLabel->setAlignment(Qt::AlignCenter);

    currentRhymeWordLabel = new QLabel(this);
    currentRhymeWordLabel->setGeometry(200, 520, 400, 40);
    currentRhymeWordLabel->setAlignment(Qt::AlignCenter);
    currentRhymeWordLabel->setStyleSheet("font-size: 22px; color: #c0392b; font-weight: bold; background: rgba(255,255,255,230); border: 2px solid #c0392b; border-radius: 8px;");
    currentRhymeWordLabel->hide();

    connect(ui->nextWordButton, &QPushButton::clicked, this, &MainWindow::onNextWordButtonClicked);

    rhymeTimer = new QTimer(this);
    connect(rhymeTimer, &QTimer::timeout, this, &MainWindow::updateRhymeWord);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::paintEvent(QPaintEvent *event) {
    QPainter painter(this);

    // Ищем рядом с exe
    QPixmap bg(QCoreApplication::applicationDirPath() + "/fon.png");
    if (bg.isNull()) bg.load(QCoreApplication::applicationDirPath() + "/fon.jpg");

    if (!bg.isNull()) painter.drawPixmap(0, 0, width(), height(), bg);
    QMainWindow::paintEvent(event);
}

void MainWindow::onNextWordButtonClicked() {
    if (rhymeRunning || persons.size() <= 1 || allRhymes.isEmpty()) return;

    int randomIdx;
    static int lastRhymeIdx = -1;

    do {
        randomIdx = QRandomGenerator::global()->bounded(allRhymes.size());
    } while (allRhymes.size() > 1 && randomIdx == lastRhymeIdx);

    lastRhymeIdx = randomIdx;
    QString selected = allRhymes[randomIdx];

    rhymeWords = selected.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (rhymeWords.isEmpty()) return;

    ui->longestWordLabel->setText("Идёт отсчёт...");
    rhymeRunning = true;
    currentWordIndex = 0;
    rhymeTimer->start(450);
}

void MainWindow::updateRhymeWord() {
    if (persons.isEmpty()) {
        rhymeTimer->stop();
        return;
    }
    if (highlightedLabel) highlightedLabel->setStyleSheet("border: none;");

    if (currentWordIndex < rhymeWords.size()) {
        QString word = rhymeWords[currentWordIndex];
        currentRhymeWordLabel->setText(word);
        currentRhymeWordLabel->show();
        speech->say(word);

        currentIndex = currentIndex % persons.size();
        highlightCurrentPerson();

        if (currentWordIndex == rhymeWords.size() - 1) {
            rhymeTimer->stop();
            QTimer::singleShot(600, this, &MainWindow::removeCurrentPerson);
        } else {
            currentWordIndex++;
            currentIndex = (currentIndex + 1) % persons.size();
        }
    }
}

void MainWindow::displayPhotosInCircle(const QList<Person> &p) {
    for (QLabel* label : photoLabels) label->deleteLater();
    photoLabels.clear();
    highlightedLabel = nullptr;

    int count = p.size();
    if (count == 0) return;

    int centerX = width() / 2, centerY = 270;
    int radius = (count > 1) ? 210 : 0;
    int photoSize = 70;

    for (int i = 0; i < count; ++i) {
        double angle = (2 * M_PI / count) * i - (M_PI / 2);
        int x = centerX + radius * cos(angle) - (photoSize / 2);
        int y = centerY + radius * sin(angle) - (photoSize / 2);

        QLabel *img = new QLabel(this);
        img->setPixmap(p[i].photo.scaled(photoSize, photoSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        img->setGeometry(x, y, photoSize, photoSize);
        img->setObjectName(p[i].name);
        img->show();
        photoLabels.append(img);

        QLabel *nameTag = new QLabel(this);
        nameTag->setText(p[i].name);
        nameTag->setGeometry(x - 20, y + photoSize + 5, photoSize + 40, 18);
        nameTag->setAlignment(Qt::AlignCenter);
        nameTag->setStyleSheet("font-size: 10px; font-weight: bold; background: rgba(255,255,255,180); border-radius: 3px; color: black;");
        nameTag->show();
        photoLabels.append(nameTag);
    }
}

void MainWindow::displayWinner() {
    if (persons.isEmpty()) return;

    ui->nextWordButton->hide();
    currentRhymeWordLabel->hide();
    for (QLabel* label : photoLabels) label->deleteLater();
    photoLabels.clear();

    ui->longestWordLabel->setText("ПОБЕДИТЕЛЬ: " + persons[0].name);
    ui->longestWordLabel->setGeometry(200, 480, 400, 60);
    ui->longestWordLabel->setStyleSheet("font-size: 24px; background: rgba(255,255,255,220); border-radius: 15px; border: 4px solid #27ae60; color: #27ae60;");

    int winSize = 200;
    int winX = (width() - winSize) / 2;
    int winY = (height() - winSize) / 2 - 50;

    QLabel *winnerImg = new QLabel(this);
    winnerImg->setPixmap(persons[0].photo.scaled(winSize, winSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    winnerImg->setGeometry(winX, winY, winSize, winSize);
    winnerImg->setStyleSheet("border: 6px solid #27ae60; border-radius: 20px; background: white;");
    winnerImg->show();
    photoLabels.append(winnerImg);

    // Венок загружается из ресурсов
    QPixmap winPix(":/win.png");
    if (!winPix.isNull()) {
        QLabel *wreath = new QLabel(this);
        int wreathSize = 250;
        wreath->setPixmap(winPix.scaled(wreathSize, wreathSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        wreath->setGeometry(winX - 25, winY - 60, wreathSize, wreathSize);
        wreath->show();
        wreath->raise();
        photoLabels.append(wreath);
    }

    // Звук фейерверка из ресурсов
    playSoundEffect("qrc:/fireworks.wav");
}

void MainWindow::removeCurrentPerson() {
    if (persons.isEmpty()) return;
    currentIndex = currentIndex % persons.size();
    QLabel *label = findChild<QLabel *>(persons[currentIndex].name);
    if (label) animateRemoval(label);

    persons.removeAt(currentIndex);
    currentRhymeWordLabel->hide();
    rhymeRunning = false;

    if (persons.size() == 1) {
        QTimer::singleShot(800, this, &MainWindow::displayWinner);
    } else {
        if (currentIndex >= persons.size()) currentIndex = 0;
        QTimer::singleShot(800, this, [this]() {
            displayPhotosInCircle(persons);
        });
    }
}

void MainWindow::animateRemoval(QLabel *l) {
    QPropertyAnimation *anim = new QPropertyAnimation(l, "geometry");
    anim->setDuration(800);
    anim->setStartValue(l->geometry());
    anim->setEndValue(QRect(width(), -100, 0, 0));
    anim->start(QAbstractAnimation::DeleteWhenStopped);

    // Звук вылета из ресурсов
    playSoundEffect("qrc:/out.wav");
}

QList<Person> MainWindow::loadPhotos(const QString &folderPath) {
    QList<Person> list;
    QDir dir(folderPath);
    if (!dir.exists()) return list;
    dir.setNameFilters({"*.png", "*.jpg", "*.jpeg"});
    for (const QFileInfo &info : dir.entryInfoList()) {
        QPixmap pix(info.absoluteFilePath());
        if (!pix.isNull()) list.append(Person(info.baseName(), pix));
    }
    return list;
}

QVector<QString> MainWindow::loadRhymesFromFolder(const QString &folderPath) {
    QVector<QString> rhymes;
    QDir dir(folderPath);
    if (!dir.exists()) return rhymes;

    dir.setNameFilters({"*.txt"});
    for (const QFileInfo &info : dir.entryInfoList()) {
        QFile file(info.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            in.setEncoding(QStringConverter::Utf8);
            QString content = in.readAll().simplified();
            if (!content.isEmpty()) {
                rhymes.append(content);
            }
            file.close();
        }
    }
    return rhymes;
}

void MainWindow::highlightCurrentPerson() {
    if (persons.isEmpty()) return;
    highlightedLabel = findChild<QLabel *>(persons[currentIndex % persons.size()].name);
    if (highlightedLabel) {
        highlightedLabel->setStyleSheet("border: 6px solid #f1c40f; border-radius: 8px;");
    }
}

void MainWindow::playSoundEffect(const QString &path) {
    QSoundEffect *s = new QSoundEffect(this);
    // Для qrc путей QSoundEffect требует использовать QUrl
    if (path.startsWith("qrc:")) {
        s->setSource(QUrl(path));
    } else {
        s->setSource(QUrl::fromLocalFile(path));
    }
    s->play();
}