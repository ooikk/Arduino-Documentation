# ThingsBoard Button-State Dashboard Widgets

The physical ESP32 button state can be published to ThingsBoard in one of two ways:

1. As an **Attribute** through `TB_ATTRIBUTES_TOPIC`.
2. As **Telemetry** through `TB_TELEMETRY_TOPIC`.

Choose **one** method for the `button` key to avoid duplicated or conflicting dashboard entries.

> **Recommendation:** Use an Attribute for the latest button state, such as `PRESSED` or `RELEASED`. Use Telemetry only when you need a historical record of button events.

## Option A: Publish as an Attribute

Publish the button state to:

```text
v1/devices/me/attributes
```

Example ESP32 code:

```cpp
client.publish(
  TB_ATTRIBUTES_TOPIC,
  "{\"button\":\"PRESSED\"}"
);
```

Example payload:

```json
{
  "button": "PRESSED"
}
```

or:

```json
{
  "button": "RELEASED"
}
```

### Best Use Case

Use Attributes when the dashboard only needs to show the **current/latest state** of the physical button.

```text
Current state:
PRESSED
```

or:

```text
Current state:
RELEASED
```

### Recommended Widgets

| Display Goal | Recommended Widget | Data Source Type | Data Key |
|---|---|---|---|
| ON/OFF visual status | LED Indicator | Attribute | `button` |
| Raw button text | Value Card / Label Card | Attribute | `button` |
| Custom colored text | Dynamic HTML Value Card | Attribute | `button` |

### LED Indicator Configuration

Configure the widget data source:

```text
Type: Attribute
Key:  button
```

Open the data-key configuration using the pencil icon and enable:

```text
Use data post-processing function
```

Paste:

```javascript
return value === "PRESSED";
```

This converts the string button state into a Boolean result:

```text
PRESSED  → true  → Indicator ON
RELEASED → false → Indicator OFF
```

### Value Card Configuration

Configure the widget data source:

```text
Type: Attribute
Key:  button
```

No post-processing is needed.

The card displays the raw text:

```text
PRESSED
```

or:

```text
RELEASED
```

### Dynamic HTML Value Card Configuration

1. Click:

   ```text
   + Add Widget → Create New Widget
   ```

2. Select:

   ```text
   Cards → HTML Value Card
   ```

3. Do not use a static HTML widget.

4. Open the:

   ```text
   Data
   ```

   tab.

5. Select the ESP32 device entity:

   ```text
   ESP32-S3-Sensor-01
   ```

6. Add a data key:

   ```text
   Type: Attribute
   Key:  button
   Label: button
   ```

Use this HTML and CSS templates:

```html
<div class="card-container">
  <span>Status: </span>
  <span class="status-text ${button}">${button}</span>
</div>
```

```css
.card-container {
  text-align: center;
  font-size: 20px;
  font-weight: bold;
  padding: 10px;
}

.status-text.PRESSED {
  color: #f44336; /* Red */
}

.status-text.RELEASED {
  color: #9e9e9e; /* Gray */
}
```

Expected display:

```text
PRESSED  → Red text
RELEASED → Gray text
```

## Option B: Publish as Telemetry

Publish the button state to:

```text
v1/devices/me/telemetry
```

Example ESP32 code:

```cpp
client.publish(
  TB_TELEMETRY_TOPIC,
  "{\"button\":\"PRESSED\"}"
);
```

Example payload:

```json
{
  "button": "PRESSED"
}
```

### Best Use Case

Use telemetry when you need a **historical time-series log** of button presses and releases.

For example:

```text
10:30:15  PRESSED
10:30:16  RELEASED
10:45:04  PRESSED
10:45:05  RELEASED
```

### Recommended Widgets

| Display Goal | Recommended Widget | Data Source Type | Data Key |
|---|---|---|---|
| Current visual status | LED Indicator | Timeseries | `button` |
| Latest raw text | Value Card / Label Card | Timeseries | `button` |
| Historical event record | Table / Timeseries Table | Timeseries | `button` |
| Custom colored text | Dynamic HTML Value Card | Timeseries | `button` |

### Telemetry LED Indicator Configuration

Configure the widget key as:

```text
Type: Timeseries
Key:  button
```

Do not select:

```text
Attribute
```

Enable data post-processing and use:

```javascript
return value === "PRESSED";
```

### Telemetry Value Card Configuration

Configure the widget key as:

```text
Type: Timeseries
Key:  button
```

The card displays the latest telemetry value:

```text
PRESSED
```

or:

```text
RELEASED
```

### Screenshot Setup for Telemetry Attribute



**Control - Switch Control**
<img width="70%" height="auto" alt="image" src="https://github.com/user-attachments/assets/ed7b6f33-145e-4c7a-add7-d83c40ea394c" />

**Control - Power Button**
<img width="70%" height="auto" alt="image" src="https://github.com/user-attachments/assets/59c4d7c5-be23-43dd-b564-dad9d4ef8997" />

**Control - Toggle Button**
<img width="70%" height="auto" alt="image" src="https://github.com/user-attachments/assets/1bb8fd9b-ca57-4472-b37d-b4b0708a9894" />


## Important: Do Not Mix the Same Key Unnecessarily

Avoid publishing the same `button` key to both topics unless there is a clear reason.

Do not do this for a simple status display:

```cpp
client.publish(
  TB_ATTRIBUTES_TOPIC,
  "{\"button\":\"PRESSED\"}"
);

client.publish(
  TB_TELEMETRY_TOPIC,
  "{\"button\":\"PRESSED\"}"
);
```

This can create confusing duplicate data sources in ThingsBoard:

```text
Attribute → button
Telemetry → button
```

The dashboard widget may then be configured with the wrong source type.

## Topic and Widget Summary

| Requirement | ESP32 Publish Topic | ThingsBoard Data Type | Recommended Widget |
|---|---|---|---|
| Display only the latest button state | `v1/devices/me/attributes` | Attribute | LED Indicator or Value Card |
| Display `PRESSED` / `RELEASED` text | `v1/devices/me/attributes` | Attribute | Value Card |
| Show a colored button-state label | `v1/devices/me/attributes` | Attribute | Dynamic HTML Value Card |
| Keep historical button-event records | `v1/devices/me/telemetry` | Timeseries | Table or Timeseries Table |
| Display the latest telemetry value | `v1/devices/me/telemetry` | Timeseries | LED Indicator or Value Card |

## Recommended Approach

For a physical ESP32 BOOT button, publish the latest state as an Attribute:

```cpp
client.publish(
  TB_ATTRIBUTES_TOPIC,
  "{\"button\":\"PRESSED\"}"
);
```

Use an LED Indicator or Value Card configured with:

```text
Type: Attribute
Key:  button
```

Use Telemetry only if you also need historical button-press records:

```cpp
client.publish(
  TB_TELEMETRY_TOPIC,
  "{\"button\":\"PRESSED\"}"
);
```
