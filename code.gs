function doPost(e) {
  try {
    // เปิด Google Sheet หน้าปัจจุบันที่ผูกกับ Script นี้
    var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
    
    // แปลงข้อมูล JSON ที่ส่งมาจาก ESP32
    var data = JSON.parse(e.postData.contents);
    
    // ดึงค่าตัวแปรต่างๆ ตามที่ ESP32 ส่งมา
    var timestamp = new Date(); // เวลาที่บันทึกข้อมูล
    var pm2_5 = data.pm2_5;
    var pm10 = data.pm10;
    var temp = data.temp;
    var humid = data.humid;
    
    // บันทึกข้อมูลลงแถวใหม่ใน Google Sheets (เรียงตามลำดับคอลัมน์)
    sheet.appendRow([timestamp, pm2_5, pm10, temp, humid]);
    
    // ส่งค่าตอบกลับ (Response) ว่าบันทึกสำเร็จ
    return ContentService.createTextOutput(JSON.stringify({"status": "success"}))
                         .setMimeType(ContentService.MimeType.JSON);
                         
  } catch (error) {
    // กรณีเกิดข้อผิดพลาด
    return ContentService.createTextOutput(JSON.stringify({"status": "error", "message": error.toString()}))
                         .setMimeType(ContentService.MimeType.JSON);
  }
}

// ฟังก์ชันทดสอบการทำงานเบื้องต้นผ่าน GET (เผื่อกดเปิดดูผ่านเว็บเบราว์เซอร์)
function doGet(e) {
  return ContentService.createTextOutput("Community-AirWatch Script is running!");
}
